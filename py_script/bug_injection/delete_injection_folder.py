import os
import shutil

def delete_modified_code_folders(root_dir):
    for root, dirs, files in os.walk(root_dir, topdown=False):
        for name in dirs:
            if name == 'Modified_Code':
                full_path = os.path.join(root, name)
                print(f"Deleting folder: {full_path}")
                shutil.rmtree(full_path)

root_directory = './HLS_Bug_Dataset'
delete_modified_code_folders(root_directory)