bugs_data = {
    "OOB": {
        "Bug": "OOB",
        "Bug_spec": "inadvertently accessing elements outside an array's valid range",
        "Bug_supp": "",
        "Bug_steps": """- Step 1: Identify an array within the HLS design that's used in a loop or conditionally accessed.
- Step 2: Find the loop or condition where the array is accessed, and intentionally set the loop's range or the condition to exceed the array's declared size."""
    },
    "INIT": {
        "Bug": "INIT",
        "Bug_spec": "inadvertently accessing an unintialized variable",
        "Bug_supp": "",
        "Bug_steps": """- Step 1: Declare a variable within the HLS design code without providing an initial value. This variable should ideally be integral to the functionality of the code, such as being used in conditional logic, arithmetic operations, or as a function parameter later in the code.
- Step 2: Directly proceed to use this variable in a significant part of the code where its value is critical for correct operation. This could be in expressions, conditions, or as an argument in a function call, without prior initialization.
- Step 3: Ensure that there are no assignments to this variable before its first use. This absence of initialization before use is the key to introducing the uninitialized variable bug."""
    },
    "SHFT": {
        "Bug": "SHFT",
        "Bug_spec": "bit shift by an out-of-bounds amount",
        "Bug_supp": "",
        "Bug_steps": """- Step 1: Identify a variable within the HLS design that is used for a bit shift operation.
- Step 2: Modify the bit shift operation to use a shift amount that is greater than the size of the variable's data type. For example, shifting an int (typically 32 bits) by more than 31 bits."""
    },
    "USE": {
        "Bug": "USE",
        "Bug_spec": "automatic extension of the sign bit during type conversion to a larger-sized data type, leading to potentially incorrect values",
        "Bug_supp": "",
        "Bug_steps": """- Step 1: Locate a variable within the HLS design that's defined with a signed, smaller-sized data type (e.g., signed char or short).
- Step 2: Perform an operation that implicitly or explicitly converts this variable to a larger signed data type (e.g., int, long) without manually managing the sign bit.
- Step 3: Use this converted variable in a calculation or assignment that relies on its value being accurately represented in the larger data type, ignoring the potential extension of the sign bit."""
    },
    "MLU": {
        "Bug": "MLU",
        "Bug_spec": "the omission of one iteration led by incorrect manual loop unrolling",
        "Bug_supp": "",
        "Bug_steps": """- Step 1: Identify a critical array used in the HLS code. 
- Step 2: Modify the boundary condition in a loop or conditional statement that accesses the array to exceed its maximum index."""
    },
    "ZERO": {
        "Bug": "ZERO",
        "Bug_spec": "inadvertently initializing a variable to zero when it should have a nonzero initializer",
        "Bug_supp": "",
        "Bug_steps": """- Step 1: Identify a variable within the HLS design that is used for the following operation and requires a nonzero initialization.
- Step 2: Modify the variable's initialization code to set its value to zero instead of the intended nonzero value."""
    },
    "BUF": {
        "Bug": "BUF",
        "Bug_spec": "inadvertently copying from the wrong half of a split buffer",
        "Bug_supp": "",
        "Bug_steps": """- Step 1: Locate a buffer in the HLS design that's intended to be split or used in two different operational phases.
- Step 2: Alter the code responsible for copying data from this buffer to ensure it incorrectly copies from the wrong half of the buffer, introducing a logical error that could lead to unexpected behavior."""
    },
    "INF": {
        "Bug": "INF",
        "Bug_spec": "an infinite loop arising from an incorrect loop termination",
        "Bug_supp": "",
        "Bug_steps": """- Step 1: Locate a loop within the HLS design where the loop's termination condition is based on a variable's value.
- Step 2: Modify the loop's termination condition or the variable it depends on inside the loop so that the condition to terminate the loop can never be met."""
    },
    "++": {
        "Bug": "*++",
        "Bug_spec": "misunderstanding of operator precedence, erroneously assuming that dereference (*) has higher precedence than postincrement (++)",
        "Bug_supp": "",
        "Bug_steps": """- Step 1: Identify a code segment where a pointer is dereferenced and incremented in the same statement.
- Step 2: Introduce a bug by placing the postincrement operator in a position that suggests incorrect precedence, causing unexpected behavior."""
    },
    "APT": {
        "Bug": "APT",
        "Bug_spec": "the parameter \"type\" in \"#pragma HLS array_partition variable=<name> type=<type> factor=<int> dim=<int>\" is wrong among cyclic, block and complete",
        "Bug_supp": "APT bugs descriptions are as follows: Cyclic partitioning distributes elements from an original array into smaller arrays by interleaving them in a cycle. Block partitioning divides an original array into smaller arrays by consecutively grouping elements into blocks. Complete partitioning decomposes the array into individual elements.For complete type partitioning, the factor is not specified. For block and cyclic partitioning, the factor= is required. The syntax of the pragma is to place the pragma in the C source within the boundaries of the function where the array variable is defined.",
        "Bug_steps": """- Step 1: Locate the #pragma HLS array_partition directive in the code.
- Step 2: Incorrectly specify the type parameter among cyclic, block, and complete, not aligning with the intended partitioning strategy. Ensure the explanation of each partition type is clear in comments or documentation."""
    },
    "FND": {
        "Bug": "FND",
        "Bug_spec": "\"factor\" in \"#pragma HLS array_partition variable=<name> type=<type> factor=<int> dim=<int>\" not divisible by loop count",
        "Bug_supp": "FND bugs descriptions are as follows: The 'factor' is essential here as it determines the size of each partition. In cyclic partitioning, it specifies the stride for interleaving; in block partitioning, it determines the size of each block. In complete partitioning, the 'factor' is not applicable as each array element is individually partitioned, rendering partition size irrelevant. For complete type partitioning, the factor is not specified. For block and cyclic partitioning, the factor= is required. The syntax of the pragma is to place the pragma in the C source within the boundaries of the function where the array variable is defined.",
        "Bug_steps": """- Step 1: Identify a loop that iterates over an array which has been partitioned using the #pragma HLS array_partition directive.
- Step 2: Set the factor parameter in a way that it's not divisible by the loop's iteration count, causing suboptimal partitioning."""
    },
    "DID": {
        "Bug": "DID",
        "Bug_spec": "incorrect \"dim\" value in \"#pragma HLS array_partition variable=<name> type=<type> factor=<int>  dim=<int>\"",
        "Bug_supp": "DID bugs descriptions are as follows: This value should be an integer ranging from 0 to <N> for an array that has <N> dimensions.For complete type partitioning, the factor is not specified. For block and cyclic partitioning, the factor= is required. The syntax of the pragma is to place the pragma in the C source within the boundaries of the function where the array variable is defined.",
        "Bug_steps": """- Step 1: Find the #pragma HLS array_partition directive applied to an array.
- Step 2: Specify an incorrect dim value, either out of the valid range for the array's dimensions or not aligning with the intended partitioning axis."""
    },
    "DFP": {
        "Bug": "DFP",
        "Bug_spec": "incorrect placement of \"#pragma HLS dataflow\"",
        "Bug_supp": """ The usual practice is to place such interface pragmas at the beginning of the function calls or before any loops or logic where the array is used. 
DFP bugs descriptions are as follows:
Here is an example to show correct dataflow position and incorrect one:\ 
void top(A[32], B[32]) {
#pragma HLS dataflow (correct)
    C[32];
    for (i ...) {
        for (j ...) {
        #pragma HLS dataflow (incorrect)
        // expression/memory access
        }
    }
    for (i ...) {
        for (j ...) {
        #pragma HLS dataflow (correct)
        for (k ...) {
        } 
        for (k ...) {
        }
    }
}
for (i ...) {
    for (j ...) {
        #pragma HLS dataflow (correct)
        sub0(A, C);
        sub1(C, B);
    }
}
}""",
        "Bug_steps": """- Step 1: Identify regions in the HLS design where parallel operations or pipelining are intended.
- Step 2: Incorrectly place the #pragma HLS dataflow inside nested loops or functions or before the memory access operation instead of at the beginning of the intended parallel region or just before parallel function calls."""
    },
    "IDAP": {
        "Bug": "IDAP",
        "Bug_spec": "inappropriate \"mode\" leading to incorrect data access pattern in \"#pragma HLS interface mode=<mode> port=<name>\"",
        "Bug_supp": "An example for IDAP bugs is that, for ap_fifo/axis, the data should be read/write only once in a sequential order. The syntax of the pragma is to place the pragma within the boundaries of the function.",
        "Bug_steps": """Step 1: Find the interface pragma declaration for an array(axis) or FIFO(ap_fifo).
Step 2: Change the mode parameter to an inappropriate mode like ap_fifo for data intended to be accessed randomly, or vice versa, to enforce a sequential access pattern on a randomly accessed data structure."""
    },
    "RAMB": {
        "Bug": "RAMB",
        "Bug_spec": "random m_axi access in '#pragma HLS interface mode=<mode> port=m_axi', resulting in non-burst AXI read/write",
        "Bug_supp": """The usual practice is to place such interface pragmas at the beginning of the function calls or before any loops or logic where the array is used. 
RAMB bugs descriptions are as follows:
m_axi interface must be access with consecutive addresses. 
Here is an example to show correct interface pragmas position and incorrect one:
void top(A[32], ...) {
    #pragma HLS interface m_axi port=A (incorrect)
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 3; j++) {
        ... = A[i + j];
    }
}
}
void top(A[32], ...) {
#pragma HLS interface m_axi port=A (correct)
    for (i = 0; i < 16; i++) {
    ... = A[i];
    }
}""",
        "Bug_steps": """- Step 1: Locate the #pragma HLS interface m_axi directive applied to an array meant for burst access.
- Step 2: Modify the access pattern in the code to access array elements non-sequentially, disrupting the intended burst mode of operation."""
    },
#     "RAMB": {
#         "Bug": "RAMB",
#         "Bug_spec": "random m_axi access in '#pragma HLS interface mode=<mode> port=m_axi', resulting in non-burst AXI read/write",
#         "Bug_supp": "The syntax of the pragma is to place the pragma within the boundaries of the function.",
#         "Bug_steps": """- Step 1: Locate the #pragma HLS interface m_axi directive applied to an array meant for burst access.
# - Step 2: Modify the access pattern in the code to access array elements non-sequentially, disrupting the intended burst mode of operation."""
#     },
    "SMA": {
        "Bug": "SMA",
        "Bug_spec": "scalar value being mapped to array interface with the port name like bram/ap_memory/m_axi/ap_fifo/axis et al. in \"#pragma HLS interface mode=<mode> port=m_axi\"",
        "Bug_supp": "The syntax of the pragma is to place the pragma within the boundaries of the function.",
        "Bug_steps": """- Step 1: Identify a scalar variable in the code.
- Step 2: Apply a #pragma HLS interface directive with mode=<array_interface> (e.g., m_axi) to the scalar variable, misrepresenting its nature in the interface mapping."""
    },
    "AMS": {
        "Bug": "AMS",
        "Bug_spec": "array value being mapped to scalar interface with the port name like ap_none/ap_vld/s_axilite et al. in \"#pragma HLS interface mode=<mode> port=m_axi\"",
        "Bug_supp": "The syntax of the pragma is to place the pragma within the boundaries of the function.",
        "Bug_steps": """- Step 1: Find an array variable within the HLS design.
- Step 2: Incorrectly apply a #pragma HLS interface with a scalar mode (e.g., ap_none) to the array, indicating a misuse of the interface type."""
    },
    "MLP": {
        "Bug": "MLP",
        "Bug_spec": "multi-level pipelining for \"#pragma HLS pipeline II=<int>\" positioning",
        "Bug_supp": """ The syntax of this pragma is to place the pragma in the C source within the body of the function or loop.
MLP bugs descriptions are as follows:
There should not exist any pipeline pragma inside of a pipeline pragma. 
Here is an example to show the incorrect pipeline pragma position :
for (i...) {
    for (j...) {
        #pragma HLS pipeline (incorrect)
        for (k...) {
            #pragma HLS pipeline
        }
    }
}""",
        "Bug_steps": """- Step 1: Select a loop structure intended for pipelining.
- Step 2: Insert a #pragma HLS pipeline within another pipelined loop, creating nested pipelining which is not supported."""
    },
    "ILNU": {
        "Bug": "ILNU",
        "Bug_spec": "unrolling non-innermost loop without full inner loop unrolling in \"#pragma HLS unroll factor=<N>\"",
        "Bug_supp": """The syntax is to place the pragma in the C source within the body of the loop to unroll.
ILNU bugs descriptions are as follows:
There should not exist any non-fully-unrolled loop inside of an unroll pragma. 
Here is an example to show correct unroll pragmas position and incorrect one :
for (i < 16) {
    for (j < 16...) {
        #pragma HLS unroll factor = 4
        for (k < 16...) {
            #pragma HLS unroll factor = 8 (incorrect)
        }
    }
}
for (i < 16) {
    for (j < 16...) {
        #pragma HLS unroll factor = 4
        for (k < 16...) {
            #pragma HLS unroll factor = 16 (correct)
        }
    }
}""",
        "Bug_steps": """- Step 1: Locate any nested loop structures within the HLS (High-Level Synthesis) design.
- Step 2: Verify the placement of the #pragma HLS unroll directive. It should be applied to both the non-inner loop and the inner loop.
- Step 3: Change the unroll pragma's parameter in the inner loop to not fully unroll."""
    }
}