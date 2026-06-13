// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
// export/Exporter.cpp : PNG capture (minimal self-contained encoder) and
//                       one-page PDF report with embedded viewport image.
#include "export/Exporter.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace Export {

// ── Framebuffer capture ───────────────────────────────────────────────────────

static std::vector<uint8_t> captureRGB(int w, int h) {
    std::vector<uint8_t> pixels(static_cast<size_t>(w) * h * 3);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    // GL origin is bottom-left; image conventions expect top-left.
    for (int row = 0; row < h / 2; ++row) {
        uint8_t* a = pixels.data() + static_cast<size_t>(row) * w * 3;
        uint8_t* b = pixels.data() + static_cast<size_t>(h - 1 - row) * w * 3;
        for (int c = 0; c < w * 3; ++c) std::swap(a[c], b[c]);
    }
    return pixels;
}

static std::string timestampName(const char* ext) {
    std::time_t t = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof buf, "cstructures_%Y%m%d_%H%M%S", std::localtime(&t));
    return std::string(buf) + ext;
}

// Nearest-neighbour downsample: pixel dimensions → target w/h.
static std::vector<uint8_t> downsample(const uint8_t* src, int sw, int sh,
                                        int dw, int dh) {
    std::vector<uint8_t> out(static_cast<size_t>(dw) * dh * 3);
    float sx = float(sw) / dw, sy = float(sh) / dh;
    for (int y = 0; y < dh; ++y) {
        for (int x = 0; x < dw; ++x) {
            int srcX = std::min(int(x * sx), sw - 1);
            int srcY = std::min(int(y * sy), sh - 1);
            const uint8_t* s = src + (srcY * sw + srcX) * 3;
            uint8_t*       d = out.data() + (y * dw + x) * 3;
            d[0] = s[0]; d[1] = s[1]; d[2] = s[2];
        }
    }
    return out;
}

// ── Minimal PNG encoder (uncompressed DEFLATE "stored" blocks) ────────────────
// Produces a valid PNG with no external dependencies.  The file is larger than
// a deflate-compressed PNG but is readable by every conformant PNG decoder.

static uint32_t pngCrc32(const uint8_t* data, size_t len) {
    static uint32_t table[256] = {};
    static bool ready = false;
    if (!ready) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j)
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        ready = true;
    }
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i)
        crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

static uint32_t adler32(const uint8_t* data, size_t len) {
    uint32_t s1 = 1, s2 = 0;
    for (size_t i = 0; i < len; ++i) {
        s1 = (s1 + data[i]) % 65521u;
        s2 = (s2 + s1)       % 65521u;
    }
    return (s2 << 16) | s1;
}

static void pushBE32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(uint8_t(x >> 24)); v.push_back(uint8_t(x >> 16));
    v.push_back(uint8_t(x >>  8)); v.push_back(uint8_t(x));
}

static void pngChunk(std::vector<uint8_t>& out, const char* type,
                     const uint8_t* data, size_t len) {
    pushBE32(out, uint32_t(len));
    size_t base = out.size();
    out.insert(out.end(), type, type + 4);
    if (data && len) out.insert(out.end(), data, data + len);
    pushBE32(out, pngCrc32(out.data() + base, 4 + len));
}

static bool writePNG(const std::string& path, const uint8_t* rgb, int w, int h) {
    size_t rowStride = static_cast<size_t>(w) * 3;

    // Raw scanlines with a leading filter-none byte per row.
    std::vector<uint8_t> raw;
    raw.reserve((1 + rowStride) * static_cast<size_t>(h));
    for (int y = 0; y < h; ++y) {
        raw.push_back(0); // filter type 0 = None
        const uint8_t* row = rgb + static_cast<size_t>(y) * rowStride;
        raw.insert(raw.end(), row, row + rowStride);
    }

    // Wrap raw data in a zlib "stored" (no-compression) stream.
    uint32_t adler = adler32(raw.data(), raw.size());
    std::vector<uint8_t> zlib;
    zlib.push_back(0x78); zlib.push_back(0x01); // CMF, FLG (low compression)
    for (size_t pos = 0; pos < raw.size(); ) {
        size_t   block = std::min(size_t(65535), raw.size() - pos);
        bool     last  = (pos + block >= raw.size());
        uint16_t len   = uint16_t(block);
        uint16_t nlen  = uint16_t(~block);
        zlib.push_back(last ? 0x01u : 0x00u); // BFINAL | BTYPE=00 (stored)
        zlib.push_back(len  & 0xFFu); zlib.push_back(uint8_t(len  >> 8));
        zlib.push_back(nlen & 0xFFu); zlib.push_back(uint8_t(nlen >> 8));
        zlib.insert(zlib.end(), raw.data() + pos, raw.data() + pos + block);
        pos += block;
    }
    pushBE32(zlib, adler); // Adler-32 checksum (big-endian)

    std::vector<uint8_t> out;
    const uint8_t sig[] = {137, 80, 78, 71, 13, 10, 26, 10};
    out.insert(out.end(), sig, sig + 8);

    // IHDR
    uint8_t ihdr[13] = {};
    ihdr[0]=uint8_t(w>>24); ihdr[1]=uint8_t(w>>16);
    ihdr[2]=uint8_t(w>> 8); ihdr[3]=uint8_t(w);
    ihdr[4]=uint8_t(h>>24); ihdr[5]=uint8_t(h>>16);
    ihdr[6]=uint8_t(h>> 8); ihdr[7]=uint8_t(h);
    ihdr[8]=8; ihdr[9]=2; // 8-bit RGB
    pngChunk(out, "IHDR", ihdr, 13);
    pngChunk(out, "IDAT", zlib.data(), zlib.size());
    pngChunk(out, "IEND", nullptr, 0);

    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(out.data()),
            static_cast<std::streamsize>(out.size()));
    return f.good();
}

// ── Public: PNG export ────────────────────────────────────────────────────────

std::string savePNG(int w, int h) {
    auto pixels = captureRGB(w, h);
    std::string fname = timestampName(".png");
    if (writePNG(fname, pixels.data(), w, h)) return fname;
    return {};
}

// ── PDF generation ─────────────────────────────────────────────────────────────
// Minimal hand-rolled PDF 1.4.  Only Type1 standard fonts are used (no
// embedding needed).  The viewport is embedded as a raw DeviceRGB image
// (no filter) at a reduced resolution to keep file sizes reasonable.

// Escape text for a PDF literal string (...).  Only ASCII 32-126 is kept.
static std::string pdfEsc(const std::string& s) {
    std::string out = "(";
    for (unsigned char c : s) {
        if (c == '(' || c == ')' || c == '\\') out += '\\';
        out += (c >= 32 && c <= 126) ? char(c) : ' ';
    }
    return out + ")";
}

static std::string fmtF(float v, int dec) {
    char buf[32];
    std::snprintf(buf, sizeof buf, dec == 1 ? "%.1f" : "%.2f", v);
    return buf;
}

// Build the PDF content stream (text + image draw calls).
static std::string buildContent(const ReportData& data,
                                float imgX, float imgY, float imgW, float imgH) {
    const float ML = 50.f, PW = 612.f, PH = 792.f;
    const float textBottom = imgY + imgH + 16.f; // leave gap above image

    std::ostringstream cs;
    cs << std::fixed;

    // Draw embedded viewport image.
    cs << "q\n"
       << imgW << " 0 0 " << imgH << " " << imgX << " " << imgY << " cm\n"
       << "/Im1 Do\nQ\n";

    // Caption above image.
    cs << "BT\n/F2 8 Tf\n1 0 0 1 " << imgX << " " << (imgY + imgH + 2.f) << " Tm\n"
       << "(Structural model viewport at time of export) Tj\nET\n";

    // Helper: horizontal rule.
    auto hline = [&](float y) {
        cs << "0.55 0.55 0.55 RG 0.5 w "
           << ML << " " << y << " m " << (PW - ML) << " " << y << " l S 0 0 0 RG\n";
    };

    // Helper: single text item at absolute position.
    auto txt = [&](float x, float y, const char* font, float sz, const std::string& s) {
        cs << "BT\n/" << font << " " << sz << " Tf\n"
           << "1 0 0 1 " << x << " " << y << " Tm\n"
           << pdfEsc(s) << " Tj\nET\n";
    };

    float y = PH - 36.f;

    // ── Title ──────────────────────────────────────────────────────────────────
    txt(ML, y, "F1", 15, "C_Structures  --  Structural Analysis Report");
    y -= 4.f; hline(y); y -= 12.f;

    // ── Summary line ───────────────────────────────────────────────────────────
    {
        std::time_t now = std::time(nullptr);
        char datebuf[32];
        std::strftime(datebuf, sizeof datebuf, "%Y-%m-%d %H:%M",
                      std::localtime(&now));
        txt(ML, y, "F2", 9,
            std::string("Date: ") + datebuf + "   Mode: " + data.mode +
            "   Nodes: " + std::to_string(data.nodeCount) +
            "   Members: " + std::to_string(data.beamCount));
        y -= 11.f;
        std::string eq = data.equilibriumOK
            ? "Equilibrium: OK  (global force residual < 0.01 N)"
            : "Equilibrium: UNBALANCED  (residual = " +
              fmtF(data.residualMag, 1) + " N)";
        txt(ML, y, "F2", 9, eq);
        y -= 11.f;
    }
    y -= 4.f; hline(y); y -= 13.f;

    // ── Support Reactions ──────────────────────────────────────────────────────
    bool hasFrame = !data.reactions.empty() && data.reactions[0].hasFrame;

    txt(ML, y, "F1", 10, "Support Reactions");
    y -= 13.f;

    // Column x-offsets and headers.
    const float reactCols[] = { ML, ML+45.f, ML+115.f, ML+185.f,
                                 ML+260.f, ML+340.f, ML+420.f };
    const char* reactHdrs[] = { "Node","Rx (N)","Ry (N)","Rz (N)",
                                 "Mx (Nm)","My (Nm)","Mz (Nm)" };
    int nRCols = hasFrame ? 7 : 4;

    cs << "BT\n/F1 8 Tf\n";
    for (int c = 0; c < nRCols; ++c)
        cs << "1 0 0 1 " << reactCols[c] << " " << y << " Tm\n"
           << pdfEsc(reactHdrs[c]) << " Tj\n";
    cs << "ET\n";
    y -= 3.f; hline(y); y -= 11.f;

    for (const auto& r : data.reactions) {
        if (y < textBottom) break;
        std::string vals[7] = {
            std::to_string(r.nodeIdx),
            fmtF(r.rx,1), fmtF(r.ry,1), fmtF(r.rz,1),
            fmtF(r.mx,1), fmtF(r.my,1), fmtF(r.mz,1)
        };
        cs << "BT\n/F2 8 Tf\n";
        for (int c = 0; c < nRCols; ++c)
            cs << "1 0 0 1 " << reactCols[c] << " " << y << " Tm\n"
               << pdfEsc(vals[c]) << " Tj\n";
        cs << "ET\n";
        y -= 12.f;
    }
    if (data.reactions.empty()) {
        txt(ML, y, "F2", 8, "(no supports defined)");
        y -= 12.f;
    }
    y -= 4.f;

    // ── Member Forces ──────────────────────────────────────────────────────────
    if (y > textBottom + 50.f) {
        hline(y); y -= 13.f;
        txt(ML, y, "F1", 10,
            hasFrame ? "Member End Forces" : "Member Axial Forces");
        y -= 13.f;

        const float mCols[] = { ML, ML+55.f, ML+140.f, ML+225.f, ML+310.f };
        const char* mHdrsT[] = { "Member","N (kN)","State" };
        const char* mHdrsF[] = { "Member","N (kN)","Vy (kN)","Mz (kNm)","State" };
        int nMCols = hasFrame ? 5 : 3;

        cs << "BT\n/F1 8 Tf\n";
        for (int c = 0; c < nMCols; ++c)
            cs << "1 0 0 1 " << mCols[c] << " " << y << " Tm\n"
               << pdfEsc(hasFrame ? mHdrsF[c] : mHdrsT[c]) << " Tj\n";
        cs << "ET\n";
        y -= 3.f; hline(y); y -= 11.f;

        for (const auto& m : data.members) {
            if (y < textBottom) break;
            const char* state = std::abs(m.N) < 50.f ? "Zero"
                              : (m.N > 0.f ? "Tension" : "Compression");
            std::string valsF[5] = {
                std::to_string(m.memberIdx),
                fmtF(m.N  * 1e-3f, 2),
                fmtF(m.Vy * 1e-3f, 2),
                fmtF(m.Mz * 1e-3f, 2),
                state
            };
            std::string valsT[3] = {
                std::to_string(m.memberIdx),
                fmtF(m.N * 1e-3f, 2),
                state
            };
            cs << "BT\n/F2 8 Tf\n";
            for (int c = 0; c < nMCols; ++c)
                cs << "1 0 0 1 " << mCols[c] << " " << y << " Tm\n"
                   << pdfEsc(hasFrame ? valsF[c] : valsT[c]) << " Tj\n";
            cs << "ET\n";
            y -= 12.f;
        }
        if (data.members.empty()) {
            txt(ML, y, "F2", 8, "(no members defined)");
        }
    }

    return cs.str();
}

// ── Public: PDF export ────────────────────────────────────────────────────────

std::string savePDF(const ReportData& data, int vpW, int vpH) {
    auto pixels = captureRGB(vpW, vpH);

    // Reduce viewport to max 640 wide to keep PDF file size manageable.
    int imgPxW = std::min(vpW, 640);
    int imgPxH = imgPxW * vpH / vpW;
    std::vector<uint8_t> imgData = (imgPxW == vpW && imgPxH == vpH)
        ? pixels
        : downsample(pixels.data(), vpW, vpH, imgPxW, imgPxH);

    // Scale image to fit the bottom portion of the US-Letter page (612×792 pts).
    const float ML = 50.f, PW = 612.f;
    const float COLTW = PW - 2.f * ML; // 512 pts
    float imgW = COLTW;
    float imgH = imgW * float(vpH) / float(vpW);
    if (imgH > 280.f) { imgH = 280.f; imgW = imgH * float(vpW) / float(vpH); }
    float imgX = ML + (COLTW - imgW) / 2.f;
    float imgY = 40.f; // bottom margin

    std::string csStr = buildContent(data, imgX, imgY, imgW, imgH);

    // ── Assemble 7 PDF objects into a binary buffer ────────────────────────────
    // Objects: 1=Catalog 2=Pages 3=Page 4=Content 5=Image 6=Font-Bold 7=Font
    std::vector<uint8_t> buf;
    std::vector<size_t>  offsets(7, 0);

    auto append = [&](const std::string& s) {
        buf.insert(buf.end(), s.begin(), s.end());
    };
    auto appendB = [&](const uint8_t* d, size_t n) {
        buf.insert(buf.end(), d, d + n);
    };

    append("%PDF-1.4\n%\xc2\xa5\xc2\xb1\xc3\xab\n"); // binary hint

    // obj 1: Catalog
    offsets[0] = buf.size();
    append("1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");

    // obj 2: Pages
    offsets[1] = buf.size();
    append("2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n");

    // obj 3: Page
    offsets[2] = buf.size();
    append("3 0 obj\n"
           "<< /Type /Page /Parent 2 0 R\n"
           "   /MediaBox [0 0 612 792]\n"
           "   /Contents 4 0 R\n"
           "   /Resources <<\n"
           "     /Font << /F1 6 0 R /F2 7 0 R >>\n"
           "     /XObject << /Im1 5 0 R >>\n"
           "   >>\n"
           ">>\nendobj\n");

    // obj 4: Content stream
    offsets[3] = buf.size();
    append("4 0 obj\n<< /Length " + std::to_string(csStr.size()) + " >>\nstream\n");
    append(csStr);
    append("\nendstream\nendobj\n");

    // obj 5: Image XObject (raw DeviceRGB, no filter)
    offsets[4] = buf.size();
    append("5 0 obj\n"
           "<< /Type /XObject /Subtype /Image\n"
           "   /Width "  + std::to_string(imgPxW) +
           " /Height " + std::to_string(imgPxH) + "\n"
           "   /ColorSpace /DeviceRGB /BitsPerComponent 8\n"
           "   /Length " + std::to_string(imgData.size()) + "\n"
           ">>\nstream\n");
    appendB(imgData.data(), imgData.size());
    append("\nendstream\nendobj\n");

    // obj 6: Font F1 (Helvetica-Bold headings)
    offsets[5] = buf.size();
    append("6 0 obj\n"
           "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold >>\n"
           "endobj\n");

    // obj 7: Font F2 (Helvetica body)
    offsets[6] = buf.size();
    append("7 0 obj\n"
           "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\n"
           "endobj\n");

    // xref table (8 entries: free head + 7 objects)
    size_t xrefOff = buf.size();
    append("xref\n0 8\n0000000000 65535 f \n");
    for (int i = 0; i < 7; ++i) {
        char entry[22];
        std::snprintf(entry, sizeof entry, "%010zu 00000 n \n", offsets[i]);
        append(entry);
    }

    // trailer
    append("trailer\n<< /Size 8 /Root 1 0 R >>\nstartxref\n");
    append(std::to_string(xrefOff) + "\n%%EOF\n");

    std::string fname = timestampName(".pdf");
    std::ofstream f(fname, std::ios::binary);
    if (!f) return {};
    f.write(reinterpret_cast<const char*>(buf.data()),
            static_cast<std::streamsize>(buf.size()));
    return f.good() ? fname : std::string{};
}

} // namespace Export
