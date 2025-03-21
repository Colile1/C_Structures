# C_Structures

# C_Structures: Project Proposal

## Real-Time 3D Structural Analysis Simulator
C_Structures is an intuitive 3D design interface for engineers and students, enabling real-time structural analysis and force visualization.

## Team
**Colile Sibanda (Sole Developer)**  
- **Role:** Full-stack developer responsible for 3D rendering, physics simulation, UI/UX, testing, and deployment.
- **Rationale:** Working alone ensures focused ownership, leveraging expertise in C++ and numerical methods while aligning with ALX’s timeline and scope constraints.

## Technologies
### Core Stack
- **Languages:** C++ (performance-critical rendering/simulation)
- **Libraries/Frameworks:**
  - SDL2 & OpenGL: Window management and 3D rendering
  - Eigen: Linear algebra for force calculations
  - GLM: Matrix/vector math for transformations
- **Build Tools:** CMake (cross-platform compilation)
- **Version Control:** Git/GitHub with GitHub Flow
- **Testing:** Google Test (unit tests), Valgrind (memory profiling)

### Alternatives Evaluated
#### OpenGL vs. Vulkan
- **Vulkan:** Lower-level control, better performance.
- **OpenGL:** Easier to learn, faster prototyping.
- **Decision:** OpenGL for rapid development within the 3-week timeline.

#### Eigen vs. Armadillo
- **Armadillo:** MATLAB-like syntax, slower for small matrices.
- **Eigen:** Template-heavy, optimized for real-time calculations.
- **Decision:** Eigen for performance with structural meshes.

## Challenge
### What Problem Does C_Structures Solve?
Traditional structural analysis tools are complex and costly. C_Structures provides:
- **Real-time 3D visualization** of forces like tension and compression.
- **Drag-and-drop design** for basic structures (e.g., beams, trusses).
- **Hands-on learning** by modifying structures and observing force redistribution.

### What C_Structures Will NOT Solve
- **Advanced Engineering Analysis:** No support for dynamic loads (e.g., earthquakes, wind) or material plasticity.
- **Production-Ready Designs:** Not a replacement for certified software like AutoCAD Civil 3D or SAP2000.

### Who Will Benefit?
- **Civil Engineering Students:** Learn shear forces and bending moments interactively.
- **Educators:** Demonstrate structural behavior in classrooms.
- **Hobbyists & DIY Enthusiasts:** Experiment with basic structural designs.

## Risks
### Technical Risks
- **Real-Time Rendering Latency:** 
  - *Impact:* Lag during interactions.
  - *Mitigation:* Simplify mesh complexity; use Level of Detail (LOD) techniques.
- **Numerical Instability:** 
  - *Impact:* Incorrect force calculations.
  - *Mitigation:* Validate Eigen solvers with unit tests.

### Non-Technical Risks
- **Solo Development Bottlenecks:** 
  - *Impact:* Delays due to unforeseen challenges.
  - *Mitigation:* Daily progress tracking via Trello; buffer time in schedule.
- **Scope Creep:** 
  - *Impact:* Missed deadlines.
  - *Mitigation:* Strict MVP definition (basic beam/truss analysis only).

## Infrastructure
- **Branching/Merging:** GitHub Flow (main branch for stable releases, feature branches for development).
- **CI/CD:** GitHub Actions for automated testing on push.
- **Deployment:**
  - Linux executable built via CMake.
  - Portable binaries distributed via GitHub Releases.
- **Data Population:**
  - Predefined structural templates (beams, trusses).
  - CSV import/export for node coordinates.
- **Testing:**
  - **Unit Tests:** Google Test for matrix math and force algorithms.
  - **Integration Tests:** Manual QA for UI/rendering interactions.

## Existing Solutions
1. **ANSYS Mechanical**
   - **Similarities:** 3D structural analysis.
   - **Differences:** ANSYS is commercial, complex, and costly; C_Structures is free, lightweight, and educational.
2. **Frame3DD**
   - **Similarities:** Static analysis of trusses/beams.
   - **Differences:** Frame3DD uses CLI/text files; C_Structures offers real-time 3D interaction.
3. **PhET Interactive Simulations**
   - **Similarities:** Visualizes basic physics concepts.
   - **Differences:** PhET targets K-12 students; C_Structures focuses on engineering-level analysis.

### Rationale
C_Structures bridges the gap between theoretical textbooks and professional tools by combining real-time 3D design with instant force feedback.


# C_Structures: MVP Specification

## Architecture
The architecture of C_Structures MVP is designed to balance real-time performance with simplicity, focusing on three core layers:

### Core Layers
- **User Interface (SDL2/OpenGL)**: Handles drag-and-drop structure creation, force input, and real-time 3D visualization.
- **Application Core (C++)**:
  - **Physics Engine**: Uses Eigen for matrix calculations (static force equilibrium).
  - **Model Manager**: Stores node/beam data (positions, forces, constraints).
- **Data Flow**:
  - User Input → Model Updates → Force Simulation → Render Loop → Visual Feedback.

Below is a diagram that illustrates how these components interact to deliver the MVP’s functionality.
```mathematica
                   ┌─────────────────────────────┐
                   │       User Interface        │
                   │  (3D Design & Simulation UI)│
                   └─────────────┬───────────────┘
                                 │
                                 │ User Actions (Design, Simulate)
                                 ▼
                   ┌─────────────────────────────┐
                   │      API & Communication    │
                   │    (RESTful endpoints)      │
                   └─────────────┬───────────────┘
                                 │
         ┌───────────────────────┼────────────────────────┐
         │                       │                        │
         ▼                       ▼                        ▼
┌────────────────┐      ┌─────────────────┐      ┌─────────────────────┐
│ Simulation     │      │  Data Model &   │      │  CSV Import/Export  │
│ Engine         │      │  Persistence    │      │  (Predefined        │
│ (C++ core,     │◄────►│  (Structure     │◄────►│   Templates, Node   │
│ OpenGL, Eigen) │      │  storage, files)│      │   coordinates)      │
└────────────────┘      └─────────────────┘      └─────────────────────┘
```
## Component Interaction Diagram

```mathematica
+-------------------+       +-------------------+       +-------------------+  
|    User Interface |       |  Application Core |       |       Renderer    |  
| (SDL2/OpenGL)     |<----->| (C++ Model/Sim)  |<----->| (OpenGL + GLM)    |  
+-------------------+       +-------------------+       +-------------------+  
        |                          |                            |  
        | User Input (Drag, Force) |                            |  
        v                          v                            v  
+-------------------+       +-------------------+       +-------------------+  
|  Event Handling   |       | Physics Engine    |       | 3D Visualization  |  
| (Mouse/Keyboard)  |       | (Eigen Solvers)   |       | (Color Mapping)   |  
+-------------------+       +-------------------+       +-------------------+  
```
## Data Flow Diagram
```mathematica
+-------------------+       +-------------------+       +-------------------+
|    User Interface |       |  Application Core |       |       Renderer    |
| (SDL2/OpenGL)     |<----->| (C++ Model/Sim)  |<----->| (OpenGL + GLM)    |
+-------------------+       +-------------------+       +-------------------+

    |                          |                            |
    | User Input (Drag, Force) |                            |
    |
    v                          v                            v
    v
    ----------------+       +-------------------+       +-------------------+
    vent Handling   |       | Physics Engine    |       |
    (Mouse/Keyboard)  |       | (Eigen Solvers)   |       |
    ----------------+       +-------------------+       +-------------------+
    |                          |                            |
    | User Input (Drag, Force) |                            |
    |
    v                          v                            v
+-------------------+       +-------------------+       +-------------------+
|  Event Handling   |       | Physics Engine    |       | 3D Visualization  |
| (Mouse/Keyboard)  |       | (Eigen Solvers)   |       | (Color Mapping)   |
+-------------------+       +-------------------+       +-------------------+
```

### Technical Stack
```mathematica

| Layer             | Technology      | Purpose                                              |
|------------------|---------------|------------------------------------------------------|
| UI/Rendering     | SDL2, OpenGL   | Window management, input handling, 3D rendering.    |
| Physics/Simulation | Eigen, GLM   | Matrix math for structural analysis.                |
| Data Management  | CSV files      | Import/export of node coordinates and beam connections. |
| Build/Deploy     | CMake, GitHub  | Cross-platform compilation and distribution.        |
```

## Architecture Overview

The architecture of C_Structures MVP is designed to balance real-time performance with simplicity, focusing on three core layers: User Interface, Application Core, and Data Flow. Below is a breakdown of how these components interact to deliver the MVP’s functionality.

### Key Components

#### **User Interface (SDL2 + OpenGL)**
**Role:** Handles user interactions (e.g., drag-and-drop design, force input) and 3D visualization.

**Subcomponents:**
- **SDL2**: Manages window creation, input events (mouse/keyboard), and UI rendering.
- **OpenGL**: Renders 3D structures (beams, nodes) and force visualizations (color-coded tension/compression).

**Features:**
- Drag-and-drop nodes/beams.
- Sliders/buttons for force magnitude and direction.
- Real-time rendering of structural deformations.

#### **Application Core (C++)**
**Role:** Manages the structural model, physics simulation, and data flow.

**Subcomponents:**
- **Physics Engine:**
  - Uses Eigen to solve static equilibrium equations (e.g., Kx = F, where K is the stiffness matrix).
  - Computes beam forces (tension/compression) and node displacements.
- **Model Manager:**
  - Stores nodes (coordinates, fixed/free status) and beams (connections, material properties).
  - Handles CSV import/export for predefined templates.
- **Renderer (OpenGL + GLM):**
  - Transforms model data into 3D meshes using GLM for matrix operations (e.g., glm::translate, glm::perspective).
  - Applies color gradients based on force magnitudes (red = compression, blue = tension).

### Data Flow

1. **User Input** (e.g., placing a node, applying a force) triggers updates in the Model Manager.
2. **Physics Engine** recalculates forces and displacements using Eigen’s linear algebra solvers.
3. **Updated Model Data** (node positions, beam forces) is passed to the Renderer.
4. **OpenGL Redraws** the scene with visual feedback (e.g., deformed beams, force colors).
5. **Loop:** Runs at 60 FPS for smooth interaction.




## Key Architectural Decisions

### **Performance vs. Complexity**
- **OpenGL over Vulkan**: Prioritized rapid development over low-level control.
- **Eigen over Armadillo**: Optimized for small, frequent matrix operations in real-time.

### **Simplicity for Solo Development**
- **Modular Design**: UI, physics, and rendering are decoupled for easier testing.
- **Minimal Dependencies**: Relies on lightweight, well-documented libraries (SDL2, Eigen).

### **Latency Mitigation**
- **Level of Detail (LOD)**: Simplified beam meshes when zoomed out.
- **Precomputed Templates**: Preloaded truss/beam designs reduce runtime calculations.

## Risks Addressed by Architecture
- **Real-Time Lag**: Asynchronous physics threading (future MVP expansion).
- **Numerical Stability**: Eigen’s robust ConjugateGradient solver with preconditioning.


---

## APIs and Methods

### Internal C++ Classes/Methods
#### **Structure Class**
- `addNode(float x, float y, float z)`: Adds a node to the model.
- `addBeam(Node* n1, Node* n2, Material material)`: Connects two nodes with a beam.
- `applyForce(Node* node, float magnitude, vec3 direction)`: Applies a force vector to a node.

#### **Simulator Class**
- `solveStaticForces()`: Uses Eigen to compute tension/compression in beams.
- `getNodeDisplacement(Node* node)`: Returns displacement after simulation.

#### **Renderer Class**
- `drawBeam(Beam beam, Color color)`: Renders beams with color-coded force visualization.

### 3rd-Party APIs
- **OpenGL (GLM)**: For matrix transformations (e.g., `glm::translate`, `glm::perspective`).
- **Eigen**: Linear algebra solver (`Eigen::ConjugateGradient` for sparse matrices).

---

## Data Model
Below is an Entity Relationship Diagram to clarify how data flows.

---

## User Stories
- **As a student**, I want to create a simple beam with fixed supports, apply a point load, and see color-coded tension/compression results so I can visualize bending moments.
- **As an educator**, I want to load a pre-built truss template, simulate forces, and project the 3D model in class to demonstrate load distribution.
- **As a hobbyist**, I want to drag-and-drop nodes to design a pergola, simulate wind load, and export node coordinates to validate DIY plans.
---


# Trello Cards for MVP Implementation

### Overview
The following tasks are organized as Trello cards to track the progress of the C_Structures MVP. Cards are arranged by priority, dependencies, and user stories, with technical specs, diagrams, and mockups included.

## Proposed Column

### Card 1: Core Rendering Pipeline Setup
**Title:** Implement SDL2/OpenGL Window with Basic 3D Rendering

- **Priority:** High (Critical Path)  
- **Dependencies:** None (Foundational)  
- **Description:**  
  - Set up an SDL2 window with OpenGL context.  
  - Render basic 3D shapes (spheres for nodes, cylinders for beams).  
  - Enable camera controls (rotate, zoom, pan).
- **Attachments:** Mockup: 3D Viewport
- **Subtasks:**  
  - SDL2 window initialization  
  - OpenGL shader setup (vertex/fragment)  
  - Camera controls (GLM for matrices)

### Card 2: Node/Beam Data Model Implementation
**Title:** Create Data Structures for Nodes, Beams, and Forces

- **Priority:** High (Depended on by Physics/UI)  
- **Dependencies:** None  
- **Description:**  
  - Define Node, Beam, and Force classes in C++.  
  - Implement CSV import/export for node coordinates.
- **Attachments:** ER Diagram, CSV Format Example

### Card 3: Physics Engine Integration
**Title:** Implement Static Force Solver with Eigen

- **Priority:** High (Critical Path)  
- **Dependencies:** Data Model (Card 2)  
- **Description:**  
  - Use Eigen to assemble stiffness matrices (K) and solve Kx = F.  
  - Calculate beam forces (tension/compression) and node displacements.
- **Attachments:** Stiffness Matrix Formula

### Card 4: Drag-and-Drop UI Tools
**Title:** Add Node/Beam Creation Tools to UI

- **Priority:** Medium  
- **Dependencies:** Rendering (Card 1), Data Model (Card 2)  
- **Description:**  
  - Left-panel toolbar for adding nodes/beams via drag-and-drop.  
  - Click interactions to connect nodes into beams.
- **Attachments:** Mockup: Toolbar

### Card 5: Force Visualization
**Title:** Render Color-Coded Beam Forces

- **Priority:** Medium  
- **Dependencies:** Physics Engine (Card 3), Rendering (Card 1)  
- **Description:**  
  - Map tension (blue) and compression (red) to beam colors.  
  - Add a color legend in the right panel.
- **Attachments:** Color Gradient Reference

### Card 6: Prebuilt Template Loader
**Title:** Add Predefined Structure Templates

- **Priority:** Low  
- **Dependencies:** Data Model (Card 2)  
- **Description:**  
  - Create CSV templates for common structures (e.g., simple beam, truss).  
  - Add UI button to load templates.
- **Attachments:** Template Mockup

### Card 7: Unit Testing Suite
**Title:** Write Google Tests for Physics Engine

- **Priority:** Medium  
- **Dependencies:** Physics Engine (Card 3)  
- **Description:**  
  - Validate stiffness matrix assembly.  
  - Test force calculations against known results.
- **Attachments:** Test Case Example

### Card 8: Build/Deployment Pipeline
**Title:** Set Up CMake & GitHub Actions

- **Priority:** Low  
- **Dependencies:** All Code Tasks  
- **Description:**  
  - Create CMakeLists.txt for cross-platform builds.  
  - Automate testing with GitHub Actions.
- **Attachments:** CMake Example, GitHub Actions Workflow

## Approved Column (Non-Technical Tasks)

### Card 9: Create Presentation
**Title:** Develop Project Presentation for Stakeholders

- **Priority:** High  
- **Dependencies:** Demo (Card 12)  
- **Description:**  
  - 5–10 minute slide deck summarizing problem statement, MVP scope, architecture, challenges, and demo.  
  - Embed screen recording or live coding.

### Card 10: Create Project Landing Page
**Title:** Build GitHub Pages Site for C_Structures

- **Priority:** Medium  
- **Dependencies:** None (Can start early)  
- **Description:**  
  - Host a static site on GitHub Pages with project overview, screenshots, installation instructions, links to repo, blog post, and demo video.

### Card 11: Write Comprehensive README.md
**Title:** Document Setup, Usage, and Contribution Guidelines

- **Priority:** High  
- **Dependencies:** Deployment (Card 8)  
- **Description:**  
  - Write README covering installation, usage, screenshots, and licensing.

### Card 12: Make Demo of the Project
**Title:** Record & Edit a 3-Minute MVP Demo Video

- **Priority:** Medium  
- **Dependencies:** Core Features Complete (Cards 1-5)  
- **Description:**  
  - Record drag-and-drop design, force application, real-time visualization, and CSV import/export.

### Card 13: Write Blog Post
**Title:** Publish a Technical Blog Post on Dev.to/Medium

- **Priority:** Low  
- **Dependencies:** Demo (Card 12)  
- **Description:**  
  - 1,000-word post covering motivation, challenges, optimizations, and lessons learned.

### Card 14: Create Project Logo
**Title:** Design a Logo for C_Structures
- **Priority:** Low
- **Dependencies:** None
- **Description:**
  - Create a logo using a vector graphics editor (e.g., Inkscape).
  - Ensure it's clear, simple, and recognizable.
  - Save as SVG for web use.

### Card 15: Finalize Project Documentation
**Title:** Update README, LICENSE, and CONTRIBUTING.md
- **Priority:** Low
- **Dependencies:** All Tasks
- **Description:**
  - Update README with latest information.
  - Update LICENSE to include any necessary permissions.
  - Update CONTRIBUTING.md with guidelines for contributors.
  - Add a section for project history and contributors.
  - Add a section for project roadmap and future plans.
  - Add a section for project acknowledgments and credits.
  - Add a section for project FAQs and troubleshooting tips.
  - Add a section for project changelog and version history.
  - Add a section for project security and privacy policies.
  - Add a section for project accessibility and inclusivity guidelines.
  - Add a section for project code of conduct.
  - Add a section for project ethics and
  

## Dependencies for Mandatory Tasks

```plaintext 
 
Card 9 (Presentation) → Card 12 (Demo)  
Card 10 (Landing Page) → Card 12 (Demo) + Card 11 (README)  
Card 11 (README) → Card 8 (Deployment)  
Card 12 (Demo) → Cards 1-5 (Core Features)  
Card 13 (Blog) → Card 12 (Demo)  
```

## CSV Format  
- Nodes: `NODE x y z fixed`  
- Beams: `BEAM start_index end_index youngs_modulus cross_section`  

### Example:  
```
NODE 0 0 0 1
NODE 3 0 0 0
BEAM 0 1 2e11 0.01
