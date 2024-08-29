import os
from openai import OpenAI
import requests
import re
import random
import shutil
import json
from typing import List, Dict, Any, Union
from langchain.schema import Document
import bug_description as bd
from collections import Counter

# weaviate
import fitz
from langchain.text_splitter import CharacterTextSplitter
from langchain.text_splitter import RecursiveCharacterTextSplitter
from langchain.schema.runnable import RunnablePassthrough
from langchain.schema.output_parser import StrOutputParser
from langchain_community.chat_models import ChatOpenAI
from langchain_openai import OpenAIEmbeddings
from langchain_community.vectorstores import FAISS
from langchain.prompts.few_shot import FewShotPromptTemplate
from langchain.prompts.prompt import PromptTemplate
from langchain.prompts.example_selector import (
    MaxMarginalRelevanceExampleSelector,
    SemanticSimilarityExampleSelector,)

# TODO: add API_key here

start_bug_type = "OOB"
start_benchmark_suite = "HIDA"
start_design = "forward_node39"

def get_response(prompt , model="gpt-4-0125-preview"):
    client = OpenAI(api_key = api_key)
    messages = [{"role": "user", "content":prompt}]
    response = client.chat.completions.create(
    model = model,
    messages = messages,
    temperature = 0.7, #0.7, # 0.7
    response_format = {"type": "json_object"})
    return response.choices[0].message.content

def count_total_tokens(file_path):
    # Regular expression to match C++ tokens: identifiers, keywords, operators, etc.
    token_pattern = r'\b[a-zA-Z_][a-zA-Z0-9_]*\b|"[^"\\]*(?:\\.[^"\\]*)*"|\+\+|--|\+=|-=|\*=|/=|==|!=|<=|>=|&&|\|\||[!%^&*+=\-/|<>(){};,]'
    token_counter = Counter()

    with open(file_path, 'r') as file:
        for line in file:
            # Removing comments
            line = re.sub(r'//.*', '', line)
            line = re.sub(r'/\*.*?\*/', '', line, flags=re.DOTALL)
            tokens = re.findall(token_pattern, line)
            token_counter.update(tokens)

    # Return the total number of tokens directly
    return sum(token_counter.values())

def fetch_and_save_url_content(url, file_path):
    res = requests.get(url)  # Assuming you're using requests or similar
    # Ensure the file is opened in write mode
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(res.text)

def extract_text_from_pdf(relative_path):
    """Extract text from a PDF file."""
    #PyPDF
    # loader = PyPDFLoader(pdf_path)
    # pages = loader.load_and_split()
    # return pages
    current_directory = os.getcwd()
    pdf_path = os.path.join(current_directory , relative_path , )
    doc = fitz.open(pdf_path)
    metadatas : List[Dict[str, Any]] = []
    pdf_texts : List[Dict[str, Any]] = []
    for i, page in enumerate(doc):
        text = page.get_text().encode('utf8').decode("utf8" , errors = 'ignore')
        metadatas.append ({
            'relative_path': relative_path,
            'pagenumber': page.number + 1, # +1 for human indexing
        })
        pdf_texts.append(text)
    return metadatas , pdf_texts

def split_file(texts: List[str] , metadatas: List[Dict[str, Any]] , split_type) -> List[Document]:
    text_splitter = choose_text_splitter(split_type)
    contexts: List[Document] = text_splitter.create_documents(texts = texts , metadatas = metadatas)
    if split_type == 1:
        contexts_pruning_url = remove_url_in_doc(contexts)
    return contexts_pruning_url

def choose_text_splitter(split_type):
    # split_type:
    # 1: pdf (separators =  "\n\n", ". ", "\n", " ", "")
    # 2: txt source codes (seperators = "\n\n", "\n", ". ", " ", "")
    if split_type==1:
        text_splitter = RecursiveCharacterTextSplitter.from_tiktoken_encoder(
        chunk_size=200,
        chunk_overlap=50,
        separators=[
            "\n\n", ". ", "\n", " ", ""
        ]  # try to split on paragraphs... fallback to sentences, then chars, ensure we always fit in context window
        )
    else:
        text_splitter = RecursiveCharacterTextSplitter.from_tiktoken_encoder(
        chunk_size=200,
        chunk_overlap=50,
        separators=[
            "\n\n", "\n", ". ", " ", ""
        ]  # try to split on paragraphs... fallback to sentences, then chars, ensure we always fit in context window
        )
    return text_splitter

def remove_url_and_before(text):
    # Regular expression pattern for matching URLs
    # This pattern is quite simple and might need adjustments to match all possible URLs accurately
    pattern = r'.*?(https://.*?\n)'
    
    # Find all matches of the pattern in the text
    matches = re.findall(pattern, text)
    
    # If a URL is found, remove the URL and everything before it
    if matches:
        # Find the first URL's position
        first_url_pos = text.find(matches[0])
        
        # Remove everything up to and including the first URL
        new_text = text[first_url_pos + len(matches[0]):]
        
        return new_text.strip()  # Remove leading and trailing whitespace
    else:
        # If no URL is found, return the original text
        return text

def remove_url_in_doc(contexts: List[Document]):
    for i in range(len(contexts)):
        contexts[i].page_content = remove_url_and_before(contexts[i].page_content)
    return contexts

def embed_documents(documents: List[Document] , api_key : str , filename : str) -> List[Document]:
    embedding = OpenAIEmbeddings(openai_api_type = api_key) # default model: str = "text-embedding-ada-002"
    text_content = []
    embedding_content = []
    for page in documents:
        text_content.append(page.page_content)
        # embedding_content.append(embedding.embed_query(page.page_content))
    write_list_to_file(text_content , filename+"_text.txt")
    # write_list_to_file(embedding_content , filename+"_embedding.txt") # it does not work
    return text_content , embedding_content

def write_list_to_file(content_list: List[Union[str, List]], file_name: str):
    # Define the directory path where the file will be saved
    current_directory = os.getcwd()
    target_directory = os.path.join(current_directory, 'RAG', 'bug_RAG_test', 'middle_level_files')
    
    # Check if the target directory exists, create it if it doesn't
    os.makedirs(target_directory, exist_ok=True)
    
    # Define the full file path
    file_path = os.path.join(target_directory, file_name)
    
    # Write the content list to the file
    with open(file_path, 'w', encoding='utf-8') as file:
        for item in content_list:
            # Check if the item is a list (or vector), and convert it to a string
            if isinstance(item, list):
                item_str = ' '.join(map(str, item))  # Convert each element to string and join with space
                file.write(item_str + '\n')
            else:
                # Item is a string, write it directly
                file.write(item + '\n')

def read_file_to_list(filename: str):
    current_directory = os.getcwd()
    file_path = os.path.join(current_directory, 'RAG', 'bug_RAG_test', 'middle_level_files', filename)
    content_list = []
    with open(file_path, 'r', encoding='utf-8') as file:
        for line in file:
            line = line.strip()  # Remove leading/trailing whitespace
            if line:  # Check if line is not empty
                # Attempt to parse the line as a vector or a string
                items = line.split()
                if all(item.replace('.', '', 1).isdigit() or item.lstrip('-').replace('.', '', 1).isdigit() for item in items):
                    # Convert to int or float based on content
                    vector = [int(item) if item.isdigit() else float(item) for item in items]
                    content_list.append(vector)
                else:
                    # Handle as a string
                    content_list.append(line)
            else:
                # If the line is empty, append an empty string to maintain line breaks
                content_list.append("")
    return content_list

# Function to read content from a file
def read_file_content(file_path):
    with open(file_path, 'r') as file:
        content = file.read()
    return content

def load_prompt_template(prompt_template_path):
    content = read_file_content(prompt_template_path)
    prompt = PromptTemplate.from_template(content)
    prompt.input_variables = ["Bug_supp_automatic" , "Bug_example" , "HLS_code"]
    return prompt
    
def replace_bug_info(prompt_template_path , hls_code_path , error_type , examples , bug_supp_doc): # Could be modified later
    # Read the prompt and HLS code from their respective files
    prompt = read_file_content(prompt_template_path)
    HLS_code = read_file_content(hls_code_path)
    # Extract the function name from the HLS code path
    function_name = os.path.basename(hls_code_path).replace('.cpp', '')
    # Replace the placeholders in the prompt
    prompt = prompt.replace('{HLS_code}' , HLS_code)
    prompt = prompt.replace('{function_name}' , function_name)
    prompt = prompt.replace('{Bug}' , bd.bugs_data[error_type]["Bug"])
    prompt = prompt.replace('{Bug_spec}' , bd.bugs_data[error_type]["Bug_spec"])
    prompt = prompt.replace('{Bug_supp_manual}' , bd.bugs_data[error_type]["Bug_supp"])
    prompt = prompt.replace('{Bug_steps}' , bd.bugs_data[error_type]["Bug_steps"])
    prompt = prompt.replace('{Bug_example}' , examples)
    if bug_supp_doc == None:
        prompt = prompt.replace('{Bug_supp_automatic}' , '')
    else:
        prompt = prompt.replace('{Bug_supp_automatic}' , bug_supp_doc)
    return prompt , HLS_code

def get_directories(path: str) -> List[str]:
    """Utility function to get directories at a given path."""
    try:
        return [d for d in os.listdir(path) if os.path.isdir(os.path.join(path, d))]
    except FileNotFoundError:
        return []

def load_example_candidate(example_candidate_bug_type : str , example_candidate_base_path : str) -> List[Dict]:
    # Create a list of dict to example candidates. The dict contains two keys: "HLS_codes" , "Bug_injection"
    example_candidates = []
    bug_type_path = os.path.join(example_candidate_base_path , example_candidate_bug_type)
    benchmark_suite_names = get_directories(bug_type_path)
    for benchmark_suite_name in benchmark_suite_names:
        benchmark_suite_path = os.path.join(bug_type_path , benchmark_suite_name)
        design_names = get_directories(benchmark_suite_path)
        for design_name in design_names:
            design_path = os.path.join(benchmark_suite_path , design_name)
            if os.path.isfile(os.path.join(design_path, "True")) or os.path.isfile(os.path.join(design_path, "True.txt")):
                source_code_path = os.path.join(design_path, "Source_Code")
                bugs_path = os.path.join(design_path, "Bugs")
                specific_bug_path = os.path.join(bugs_path , "bug_" + example_candidate_bug_type + ".txt")
                specific_source_path = os.path.join(source_code_path , design_name + '.cpp')
                if os.path.isfile(specific_bug_path) and os.path.isfile(specific_source_path):
                    bug_json = read_file_content(specific_bug_path)
                    source_code = read_file_content(specific_source_path)
                    example_candidates.append({
                        "HLS_codes": source_code,
                        "Bug_injection": bug_json
                    })
    return example_candidates

def select_and_format_examples_with_mmr(hls_source_code , examples : List[Dict] , k=4 ):
    # Initialize the MMR example selector with your examples
    mmr_example_selector = MaxMarginalRelevanceExampleSelector.from_examples(
        examples = examples,
        embeddings = OpenAIEmbeddings(),
        vectorstore_cls = FAISS,
        k=k,
    )
    HLS_source_code_dict = {
                        "HLS_codes": hls_source_code,
                        # "Bug_injection": bug_json
                    }
    selected_examples = mmr_example_selector.select_examples(HLS_source_code_dict)
    formatted_outputs = "\n".join([f"Bug_injection: {example['Bug_injection']}" for example in selected_examples])
    return formatted_outputs

def batch_process(bug_type : List[str] , bug_generation_path : str , prompt_template_path , examples : List[Dict] , bug_supp_doc):
    # TODO: How to modify it to align it with Condition 2 and Condition 3
    if bug_type[0] == start_bug_type:
        flag = False
    else:
        flag = True
    # Walk through all the designs contained in the directory of source_code_path
    benchmark_suite_names = get_directories(bug_generation_path)
    for benchmark_suite_name in benchmark_suite_names:
        benchmark_suite_path = os.path.join(bug_generation_path , benchmark_suite_name)
        design_names = get_directories(benchmark_suite_path)
        for design_name in design_names:
            if benchmark_suite_name == start_benchmark_suite and design_name == start_design:
                flag = True
            if flag == True:
                output_path = os.path.join(benchmark_suite_path , design_name)
                source_code_path = os.path.join(output_path , 'Source_Code' ,design_name+'.cpp')
                hls_source_code = read_file_content(source_code_path)
                formatted_examples = select_and_format_examples_with_mmr(hls_source_code=hls_source_code , examples=examples)
                # Construct the format
                prompt , HLS_code = replace_bug_info(prompt_template_path , source_code_path , bug_type[0] , formatted_examples , bug_supp_doc)
                GPT_interface(prompt = prompt , bug_type = bug_type , output_path = output_path)

def GPT_interface(prompt : str , bug_type : List[str] , output_path : str):
    # Check if the file already exists
    error_filename = 'bug_' + '_'.join(bug_type) + '.txt'
    bugs_dir = os.path.join(output_path, "Bugs")
    error_file_path = os.path.join(bugs_dir, error_filename)

    if os.path.exists(error_file_path):
        print(f"File {error_file_path} already exists. Skipping GPT-4 Turbo API call.")
        return
    print(f"File {error_file_path} is calling Turbo GPT-4 API.")
    # Get the response for the generated prompt
    response = get_response(prompt=prompt)
    # print('prompt: ' , prompt)
    # print('response: ' , response) # for testing
    try:
        response_data = json.loads(response)
        temp = response_data is not None
    except json.JSONDecodeError as e:
        print("JSON decode error:", e)
        temp = False
    if temp:
        all_errors_validated = True
        # Validate each error in the response
        error_types_spec = bug_type.copy()
        for error in response_data.get("Error Specification", []):
            code_original = error["Original Code"]
            code_w_error = error["Faulty Code"]
            injected_bug = error["Error Type"]
            if check_fault_injection(code_original, code_w_error):
                all_errors_validated = False
                break
            if check_fault_type(injected_bug , error_types_spec):
                error_types_spec.remove(injected_bug)
        # Save files only if all errors are validated
        if all_errors_validated and (response_data["Error Size"] != 0):
            if error_types_spec == []:
                # Create directories if they don't exist
                os.makedirs(bugs_dir, exist_ok=True)
                save_code_to_file(bugs_dir , error_filename , json.dumps(response_data, indent=4))

def check_fault_injection(original_code , faulty_code):
    # used for check whether the fault has been injected
    # pass
    return original_code == faulty_code

def check_fault_type( injected_bug , intended_bug ):
    return injected_bug in intended_bug

def save_code_to_file(directory, filename, code):
    """Saves the given code to a file in the specified directory."""
    if not os.path.exists(directory):
        os.makedirs(directory)
    file_path = os.path.join(directory, filename)
    with open(file_path, 'w') as file:
        file.write(code)

def get_bug_type(bug_file):
    return os.path.basename(bug_file).split('_')[1].split('.')[0]

def sample_bugs(src_dir, HLS_benchmark_suites, dest_dir, total_samples, token_threshold):
    # Traverse the source directory of the part containing only source_code. Still need to add the bug_xxx.txt into the file folder.
    source_files = []
    for suite in HLS_benchmark_suites:
        suite_path = os.path.join(src_dir, suite)
        
        if os.path.isdir(suite_path):
            for root, dirs, files in os.walk(suite_path):
                for file in files:
                    if file.endswith('.cpp'):
                        full_file_path = os.path.join(root, file)
                        size_tokens = count_total_tokens(full_file_path)
                        if size_tokens <= token_threshold:
                            source_files.append(full_file_path)

    # Sample source files
    if len(source_files) > total_samples:
        sampled_sources = random.sample(source_files, total_samples)
    else:
        sampled_sources = source_files

    # Copy sampled source files and create 'Bugs' directories under dest_dir
    for source_file in sampled_sources:
        # Construct the new path under 'Bugs' directory
        relative_path = os.path.relpath(source_file, src_dir)
        design_name = relative_path.split('\\')[-1].split('.')[0]  # Splits the string by the backslash and takes the last element
        new_source_dir = os.path.join(dest_dir, os.path.dirname(relative_path) , design_name , "Source_Code")
        os.makedirs(new_source_dir, exist_ok=True)
        shutil.copy(source_file, new_source_dir)
        

def main():
    # Set default OpenAI key
    current_directory = os.getcwd()
    prompt_path = os.path.join( current_directory , "GPT4_API" , "prompt_opt" , "prompt_v1" , "prompt_ICL_RAG_CoT.txt")
    # prompt_path = os.path.join( current_directory , "GPT4_API\prompt\prompt_basic_2.txt")
    example_path = os.path.join( current_directory , "Sampling_HLS_Bug_Dataset" , "Example_Database")
    HLS_benchmark_suites = ["CHStone", "finn-hlslib", "gnn-builder", "H.264", "hls4ml", "MachSuite", "Open-Source-IPs", "polybench", "rosetta", "tacle-bench", "Vitis_Libraries", "Vitis-HLS-Introductory-Examples", "HIDA" ,"misc"]
    # HLS_benchmark_suites = ['HIDA']
    all_error_types = [ ["OOB"] , ["INIT"] , ["SHFT"] , ["INF"] , ["MLU"] , ["BUF"] , ["ZERO"] , ["USE"] , ["APT"] , ["FND"] , ["DID"] , ["DFP"] , ["IDAP"] , ["RAMB"] , ["SMA"] , ["AMS"] , ["MLP"] ]
    # all_error_types = [["ILNU"]]
    RAG_choice = 1 #1: only adding selected examples; 2: only adding HLS tutorials; 3: both adding selected examples and HLS tutorials

    # Sample some source files to a new directory '/Sampling_HLS_Bug_Dataset'
    src_dir = os.path.join( current_directory , 'HLS_Source_Dataset')
    dest_dir = os.path.join( current_directory , 'Sampling_HLS_Bug_Dataset' , 'Sample_26')
    total_samples = 5
    token_threshold = 1500
    seed_value = 166 #876 #101 #99875 #1666 #4567  # You can choose any number as your seed
    random.seed(seed_value)
    sample_bugs(src_dir, HLS_benchmark_suites, dest_dir, total_samples, token_threshold)
    flag = False

    if RAG_choice == 1 or RAG_choice == 3: # adding selected examples by MMR
        # walk through all the bug_type, load the example candidates, and then generate prompts
        for error_type in all_error_types:
            if error_type[0] == start_bug_type:
                flag = True
            if flag == True:
                example_candidates: List[Dict] = load_example_candidate(example_candidate_bug_type = error_type[0] , example_candidate_base_path = example_path)
                batch_process(bug_type = error_type, bug_generation_path = dest_dir , prompt_template_path = prompt_path , examples = example_candidates , bug_supp_doc = None)

    if RAG_choice == 2 or RAG_choice == 3: #TODO
        split_type = 1
        filename = "HLS_pragma_explanation"

        # Texts to be digested
        RAG_Doc1_relative_DIR = os.path.join("RAG" , "bug_RAG_test" , "RAG_docs" , filename+'.pdf')
        metadatas , doc1 = extract_text_from_pdf(RAG_Doc1_relative_DIR)
        
        # Chunk the document
        context = split_file(doc1, metadatas, split_type)

        # Text embedding
        # context_txt, embed_vectors = embed_documents(context , OPENAI_API_KEY , filename)
        db = FAISS.from_documents(context, OpenAIEmbeddings()) # TODO: If there is only one database, how could I add more documents into the database?

        # Similarity search
        retriever = db.as_retriever()
        docs = retriever.get_relevant_documents("what did he say about ketanji brown jackson") # need to be replaced
        print(docs[0].page_content)

    main()