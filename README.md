# Chrysalis-HLS

## Overview
The Chrysalis dataset is a comprehensive collection specifically designed for the field of High-Level Synthesis (HLS). It uniquely includes both correct source codes and deliberately injected buggy codes, providing an invaluable resource for training and improving Large Language Models (LLMs) in code debugging. This dataset is derived from over 1000 function-level designs, sourced from 12 different open-source HLS benchmark suites. Each design in the dataset is subjected to a controlled injection of up to 40 distinct combinations of bugs, significantly expanding the scope of HLS error models currently available.

## Repository Layout
### `HLS_Bug_Dataset/`
This directory is the core of the Chrysalis dataset. It contains two primary types of files:
- **Source_Code**: This file folder contains the original, correct source code files.
- **Bugs**: The files in this file folder are used to inject bugs into the design source files. Each bug file corresponds to a specific type of bug.

### `py_script/bug_injection/`
This directory contains Python scripts that are essential for manipulating the dataset:
- **bug_injection_One_Type.py, bug_injection_Two_Type.py**: These scripts are used to introduce bugs into the source code. They can inject either one type or two types of bugs, depending on the requirements of the experiment or training session.
- **delete_injection_folder**: This script is used to delete all modified codes, reverting the dataset back to its original state.

---
