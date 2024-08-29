import os
import json
import re

# Function to check for the error based on the conditions provided
def process_ZERO_bug(file_path):
    error_json = read_json_file_content(file_path)
    for error in error_json["Error Specification"]:
        error_type = error["Error Type"]
        original_code = error["Original Code"]
        modified_code = error["Faulty Code"]
        # Check if the error type is "ZERO"
        if error_type == "ZERO":
            # Check if '0' or 'zero' exist in the "Original Code"
            if '0' not in modified_code:
                # Generate the new file name
                file_dir, file_name = os.path.split(file_path)
                original_name, file_extension = os.path.splitext(file_name)
                new_file_name = f"{original_name}(delete_ZERO){file_extension}"
                new_file_path = os.path.join(file_dir, new_file_name)

                # Rename the file
                os.rename(file_path, new_file_path)
                print(f"File in {file_path} renamed to {new_file_name}")

def process_BUF_bug(file_path):
    error_json = read_json_file_content(file_path)
    for error in error_json["Error Specification"]:
        error_type = error["Error Type"]
        original_code = error["Original Code"]
        modified_code = error["Faulty Code"]
        # Check if the error type is "ZERO"
        if error_type == "BUF":
            # Check if '=' exist in the "Original Code"
            if "=" in original_code and "=" in modified_code:
                original_after_equal = original_code.split("=" , maxsplit = 1)[1]
                modified_after_equal = modified_code.split("=" , maxsplit = 1)[1]
                if original_after_equal == modified_after_equal:
                    file_dir, file_name = os.path.split(file_path)
                    original_name, file_extension = os.path.splitext(file_name)
                    new_file_name = f"{original_name}(delete_BUF){file_extension}"
                    new_file_path = os.path.join(file_dir, new_file_name)
                    # Rename the file
                    os.rename(file_path, new_file_path)
                    print(f"File in {file_path} renamed to {new_file_name}")

def process_pragma_bug(file_path):
    pragma_bug_types = ["APT","FND","DID","DFP","IDAP","RAMB","SMA","AMS","MLP","ILNU"]
    error_json = read_json_file_content(file_path)
    pragma_pattern = r"^#pragma HLS\s+[^\s]+"
    function_decl_pattern = (
        r'^\s*'                                # Optional leading spaces
        r'(?!for\s*\()'                        # Negative lookahead to exclude 'for('
        r'(?:\b(?:void|int|float|char|struct)\b\s*[*&]?\s+|\w+\s+\w+\s*[*&]?\s+)'  # Return type, possibly including pointers/references
        r'[\w_]+\s*'                           # Function name
        r'\(\s*[^)]*?\s*\)'                    # Parameters (non-greedy)
        r'\s*\{?'                              # Optional space and opening brace
    )

    combined_pattern = pragma_pattern + r"\s*" + function_decl_pattern
    combined_regex = re.compile(combined_pattern, re.MULTILINE)

    for error in error_json["Error Specification"]:
        error_type = error["Error Type"]
        original_code = error["Original Code"]
        faulty_code = error["Faulty Code"]
        # Check if the error type is "++"
        if error_type in pragma_bug_types:
            flag_decl_pattern = re.match(function_decl_pattern, original_code.strip())
            flag_pragma_pattern = re.match(pragma_pattern , faulty_code.strip()) or re.match(combined_pattern, faulty_code.strip(), re.DOTALL)
            if flag_decl_pattern:
                # TODO: find whether faulty copy is like '#pragma...' or '#pragma...' + 'function_decl_pattern' 
                if flag_pragma_pattern:
                    # Generate the new file name
                    file_dir, file_name = os.path.split(file_path)
                    original_name, file_extension = os.path.splitext(file_name)
                    new_file_name = f"{original_name}(delete_pragma){file_extension}"
                    new_file_path = os.path.join(file_dir, new_file_name)
                    # Rename the file
                    os.rename(file_path, new_file_path)
                    print(f"File in {file_path} renamed to {new_file_name}")

# Function to read content from a file
def read_file_content(file_path):
    with open(file_path, 'r') as file:
        content = file.read()
    return content

# Function to read json content from a file
def read_json_file_content(file_path):
    try:
        bug_content = read_file_content(file_path)
        bug_json = json.loads(bug_content)
        return bug_json
    except json.JSONDecodeError as json_err:
        print(f"Error parsing JSON from file {file_path}: {json_err}")
        return None
    except FileNotFoundError as fnf_err:
        print(f"File not found: {fnf_err}")
        return None
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return None

def main():
    # Decide on which bug type requires to be post-processing
    pragma_bug_types = ["APT","FND","DID","DFP","IDAP","RAMB","SMA","AMS","MLP","ILNU"]
    current_directory = os.getcwd()
    # sample_directory = os.path.join(current_directory , "Sampling_HLS_Bug_Dataset" , "HIDA")
    sample_directory = os.path.join(current_directory , "Sampling_HLS_Bug_Dataset" , "Sample_27")
    for root, dirs, files in os.walk(sample_directory):
        for file in files:
            # Check if the file ends with .txt and matches the format bug_xxx.txt
            if file.endswith(".txt") and file.startswith("bug_") and "(delete)" and "(delete_++)" and "(delete_ZERO)"  and "(delete_BUF)" and "(delete_pragma)" not in file:
                full_path = os.path.join(root, file)  # Get the full path
                # Prune out '++' bugs
                if file == "bug_++.txt":
                    process_plus_plus_bug(full_path)  # Pass the full path to the processing function
                elif file == "bug_ZERO.txt":
                    process_ZERO_bug(full_path)  # Pass the full path to the processing function
                elif file == "bug_BUF.txt":
                    process_BUF_bug(full_path)  # Pass the full path to the processing function
                else:
                    for bug_type in pragma_bug_types:
                        if file == f"bug_{bug_type}.txt":
                            process_pragma_bug(full_path)  # Pass the full path and bug type to the processing function


if __name__ == "__main__":
    main()