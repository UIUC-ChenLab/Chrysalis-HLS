import os
import json
import re

def normalize_code(code):
    if isinstance(code, str):
        code = code.strip('"""')
        # Remove block comments '/*...*/'
        code = re.sub(r'/\*.*?\*/', '', code, flags=re.DOTALL)
        # Remove single line comments '//...'
        code = re.sub(r'//.*\n', '', code)
        code = re.sub(r'/\*.*?\*/', '.*', code)
        code = code.replace('...', '.*')
        # Ensure space before '(' and '{'
        code = re.sub(r'(?<!\s)\(', ' (', code)
        code = re.sub(r'(?<!\s)\{', ' {', code)
        # Add a single space of indentation at the beginning of each line if not present
        code = re.sub(r'(?m)^(?!\s)', ' ', code)
        # Normalize whitespace to a single space within lines, but keep line breaks
        code = re.sub(r'[ \t]+', ' ', code)
        code = re.sub(r'\{ \.\*.*', '', code)
        code = re.sub(r'\.\*.*', '', code)
        code = re.sub(r'\)\n \{', ') {', code)
        code = re.sub(r'(\S)\n (\s\S)', r'\1\2' , code)
        code = code.strip()
        return code
    else:
        # Handle the case where code is not a string, or convert it to a string
        pass

def normalize_code_space(code):
    if isinstance(code, str):
        code = code.strip('"""')
        # Replace '...' with a regex pattern to match any characters
        code = re.sub(r'/\*.*?\*/', '.*', code)
        code = code.replace('...', '.*')
        # Ensure space before '(' and '{'
        code = re.sub(r'(?<!\s)\(', ' (', code)
        code = re.sub(r'(?<!\s)\{', ' {', code)
        # Add a single space of indentation at the beginning of each line if not present
        code = re.sub(r'(?m)^(?!\s)', ' ', code)
        # Normalize whitespace to a single space within lines, but keep line breaks
        code = re.sub(r'[ \t]+', ' ', code)
        # Remove contents after '.*'
        code = re.sub(r'\.\*.*', '', code)
        # Normalize whitespace to a single space
        return re.sub(r'\s+', ' ', code.strip())
    else:
        # Handle the case where code is not a string, or convert it to a string
        pass

def is_snippet_in_source(original_code, source_code):
    # Convert the normalized original code into a regular expression
    if not isinstance(original_code, str):
        return False
    pattern = re.escape(original_code).replace(r'\.\.\.', '.*')
    pattern = pattern.replace('\\.\\*', '.*')
    pattern = re.sub(r'\\ ', r'\\s+', pattern) 
    # Use re.DOTALL to allow '.' to match newlines as well
    return re.search(pattern, source_code, re.DOTALL) is not None

# def remove_dot_star_and_after(text):
#     # This regular expression matches '.*' and everything after it
#     pattern = r"\..*"
#     # Replace the matched pattern with an empty string
#     cleaned_text = re.sub(pattern, "", text)
#     return cleaned_text

def replace_code_in_source(original_code, faulty_code, source_code):
    # Use regular expressions for replacement
    # code_after_modification = source_code.replace(original_code, faulty_code)
    code_after_modification = source_code.replace(original_code, faulty_code)
    # Check if any replacement has been made
    if code_after_modification != source_code:
        return code_after_modification
    else:
        return None

def make_readable(input_str):
    """Converts string-encoded newline characters into actual newline characters."""
    return input_str.replace('\n', f'''
                             ''')

def extract_bug_types(filename):
    # Regular expression to match sequences of uppercase letters possibly separated by underscores
    matches = re.findall(r'bug_([A-Z]+(?:_[A-Z]+)*)', filename)
    if matches:
        # Split the found matches by underscores and flatten the list
        return [item for match in matches for item in match.split('_')]
    else:
        return None

def check_filename_pattern(filename):
    # Pattern for 'bug_{bugtype1}.txt'
    pattern1 = r'^bug_[^_]+\.txt$'
    # Pattern for 'bug_{bugtype1}_{bugtype2}.txt'
    pattern2 = r'^bug_[^_]+_[^_]+\.txt$'
    if re.match(pattern1, filename):
        return True
    else:
        return False
    
def check_bug_size(filename):
    num_count = filename.count('_')
    if num_count==2:
        return True
    else:
        return False

def format_code(code):
    formatted_code = ""
    indent_level = 0
    indent_size = 2  # Number of spaces for each indent level

    i = 0
    while i < len(code):
        char = code[i]

        if char == '{':
            formatted_code += ' {\n'
            indent_level += 1
            formatted_code += ' ' * (indent_level * indent_size)
        elif char == '}':
            indent_level -= 1
            formatted_code += '\n' + ' ' * (indent_level * indent_size) + '}'
        elif char == ';':
            formatted_code += ';\n' + ' ' * (indent_level * indent_size)
        else:
            formatted_code += char

        i += 1

    return formatted_code.strip()

def format_code_space(code):
    formatted_code = ""
    indent_level = 0
    indent_size = 2  # Number of spaces for each indent level

    i = 0
    while i < len(code):
        char = code[i]

        if char == '{':
            formatted_code += ' {\n'
            indent_level += 1
        elif char == '}':
            indent_level -= 1
            formatted_code = formatted_code.rstrip()  # Remove trailing spaces
            formatted_code += ' ' * (indent_level * indent_size) + '}'
            # Check if the next non-whitespace character is not '}'
            next_char_index = i + 1
            while next_char_index < len(code) and code[next_char_index].isspace():
                next_char_index += 1
            if next_char_index < len(code) and code[next_char_index] != '}':
                formatted_code += '\n'
        elif char == ';':
            formatted_code += ';\n'
        else:
            if i == 0 or code[i - 1] in '{;}':
                formatted_code += ' ' * (indent_level * indent_size)
            formatted_code += char

        if char not in '{;}':
            while i + 1 < len(code) and code[i + 1] not in '{;}':
                i += 1
                formatted_code += code[i]

        i += 1

    # Insert a newline before 'return' if it's not already preceded by '}'
    # formatted_code = formatted_code.replace('return', '\nreturn')

    return formatted_code.strip()


def main():
    current_directory = os.getcwd()
    HLS_benchmark_suites = ["CHStone", "finn-hlslib", "gnn-builder", "H.264", "hls4ml", "MachSuite", "misc", "Open-Source-IPs", "polybench", "rosetta", "tacle-bench", "Vitis_Libraries", "Vitis-HLS-Introductory-Examples"]

    Start_source_file = "abs.cpp"
    Start_bug_file = "bug_BUF_USE.txt"

    flag_Start = False

    for suite in HLS_benchmark_suites:
        HLS_DIR = os.path.abspath(os.path.join(current_directory, "HLS_Bug_Dataset", suite))

        for root, dirs, files in os.walk(HLS_DIR):
            for dir in dirs:
                bugs_dir = os.path.join(root, dir, "Bugs")
                source_code_dir = os.path.join(root , dir , "Source_Code")
                source_code_files = os.listdir(source_code_dir)
                modified_code_dir = os.path.join(root , dir , "Modified_Code")
                if not source_code_files:
                    print(f"Directory {source_code_dir} is empty!")
                    continue
                source_code_path = os.path.join(source_code_dir, source_code_files[0])
                if os.path.exists(bugs_dir):
                    with open(source_code_path, 'r') as file:
                        source_code = file.read()
                    for bug_file in os.listdir(bugs_dir):
                        Is_Two_Type = check_bug_size(bug_file)
                        bug_file_path_full = os.path.join(bugs_dir, bug_file)
                        if "(delete)" not in bug_file and "(delete3)" not in bug_file:
                            bug_file_path = os.path.join(bugs_dir, bug_file)
                            bug_type = extract_bug_types(bug_file)
                            if Start_source_file == source_code_files[0] and Start_bug_file in bug_file_path:
                                flag_Start = True
                            if flag_Start and Is_Two_Type:
                                with open(bug_file_path, 'r') as file:
                                    bug_data = json.load(file)
                                bug_sizes = bug_data['Error Size']
                                for i in range(bug_sizes):                                        
                                    if "Original Code" in bug_data["Error Specification"][i]:
                                        original_code = bug_data["Error Specification"][i]["Original Code"]
                                        if i == 1:
                                            source_code = modified_source_code
                                        faulty_code = bug_data["Error Specification"][i]["Faulty Code"]
                                        original_code_normalized = normalize_code(original_code)
                                        faulty_code_normalized = normalize_code(faulty_code)
                                        source_code_normalized = normalize_code(source_code)
                                        if is_snippet_in_source(original_code_normalized, source_code_normalized):
                                            if original_code_normalized in source_code_normalized:
                                                modified_source_code = replace_code_in_source(original_code_normalized, faulty_code_normalized, source_code_normalized)
                                            # formatted_modified_source_code = format_code(modified_source_code)
                                            elif 'pragma' not in source_code_normalized:
                                                modified_source_code = replace_code_in_source(normalize_code_space(original_code_normalized), faulty_code_normalized, normalize_code_space(source_code_normalized))
                                                test_modified_source_code = modified_source_code
                                                modified_source_code = format_code_space(modified_source_code)
                                            if modified_source_code == None:
                                                break
                                if modified_source_code != None:
                                    bug_type_str = '_'.join(bug_type)
                                    source_code_file = source_code_files[0]
                                    new_file_name = os.path.splitext(source_code_file)[0] + "_" + bug_type_str + os.path.splitext(source_code_file)[1]
                                    new_file_path = os.path.join(modified_code_dir, new_file_name)

                                    # Ensure the directory exists
                                    os.makedirs(modified_code_dir, exist_ok=True)

                                    # Check if the file already exists
                                    if not os.path.exists(new_file_path):
                                        with open(new_file_path, 'w') as new_file:
                                            new_file.write(modified_source_code)
                                        print(f"Modified source code written to {new_file_path}")
                                    else:
                                        print(f"File {bug_file_path_full} already exists. Skipping.")
                                else:
                                    print(f"{bug_file_path_full} isn't modified yet!")
                        else:
                            # print(f"{bug_file_path_full} has been skipped or deleted")
                            pass
                else:
                    # print(f"{bug_file_path_full} does not exist")
                    pass
            break

if __name__ == "__main__":
    main()