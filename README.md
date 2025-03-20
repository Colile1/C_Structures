# C_Structures

## CSV Format  
- Nodes: `NODE x y z fixed`  
- Beams: `BEAM start_index end_index youngs_modulus cross_section`  

### Example:  
```
NODE 0 0 0 1
NODE 3 0 0 0
BEAM 0 1 2e11 0.01
