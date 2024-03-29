# Chrysalis-HLS

## Overview
The Chrysalis dataset is a comprehensive collection specifically designed for the field of High-Level Synthesis (HLS). It uniquely includes both correct source codes and deliberately injected buggy codes, providing an invaluable resource for training and improving Large Language Models (LLMs) in code debugging. This dataset is derived from over 1000 function-level designs, sourced from 12 different open-source HLS benchmark suites. Each design in the dataset is subjected to a controlled injection of up to 40 distinct combinations of bugs, significantly expanding the scope of HLS error models currently available.

## Bug Type Specification
- **'OOB'**: Out-of-bounds array access
- **'INIT'**: Read of uninitialized variable
- **'SHFT'**: Bit shift by an out-of-bounds amount
- **'INF'**: An infinite loop arising from an incorrect loop termination
- **'USE'**: Unintended sign extension
- **'MLU'**: Errors in manual loop unrolling
- **'ZERO'**: Variable initialized to zero instead of nonzero initializer 
- **'BUF'**: Copying from the wrong half of a split buffer
- **'APT'**: Array partition type error for the pragma '#pragma HLS array_partition'
- **'FND'**: Factor not divisible for the pragma '#pragma HLS array_partition'
- **'DID'**: Dim incorrect dimension for the pragma '#pragma HLS array_partition'
- **'DFP'**: Dataflow position error for the pragma '#pragma HLS dataflow'
- **'IDAP'**:	Incorrect data access pattern for the pragma '#pragma HLS interface'
- **'RAMB'**:	m_axi interface is accessed randomly in the code, resulting in non-burst AXI read/write for the pragma '#pragma HLS interface'
- **'SMA'**: Scalar values mapped to array interfaces like bram/ap_memory/m_axi/ap_fifo/axis for the pragma '#pragma HLS interface'
- **'AMS'**: Array value mapped to scalar interfaces like ap_none/ap_vld/s_axilite for the pragma '#pragma HLS interface'
- **'MLP'**: Multi-level pipelining for the pragma '#pragma HLS pipeline'
- **'ILNU'**: Inner loops not fully-Unrolled for the pragma '#pragma HLS unroll'

## Repository Layout
`HLS_Bug_Dataset/`
This directory is the core of the Chrysalis dataset. It contains two primary types of files:
- **Source_Code**: This file folder contains the original, correct source code files.
- **Bugs**: The files in this file folder are used to inject bugs into the design source files. Each bug file corresponds to a specific type of bug.

`py_script/bug_injection/`
This directory contains Python scripts that are essential for manipulating the dataset:
- **bug_injection_One_Type.py, bug_injection_Two_Type.py**: These scripts are used to introduce bugs into the source code. They can inject either one type or two types of bugs, depending on the requirements of the experiment or training session.
- **delete_injection_folder**: This script is used to delete all modified codes, reverting the dataset back to its original state.

---
