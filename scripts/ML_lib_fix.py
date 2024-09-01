import shutil
import os
Import("env")

print("Running ML fix action")

# List of directories to process
directories = ["esp32s3", "esp32s2", "esp32c3", "esp32"]

# Loop through each directory
for dir_name in directories:
    print(f"Processing directory: {dir_name}")
    
    # Get path to the ML library which is located in lib
    ml_lib_path = os.path.join(env.subst("$PROJECT_DIR"), f".pio/libdeps/{dir_name}/ML_SynthTools/src/{dir_name}/")
    ml_lib_name = "ML_SynthTools.a"
    ml_lib_name_new = "lib" + ml_lib_name
    
    # Construct full paths for the source and destination
    source_path = os.path.join(ml_lib_path, ml_lib_name)
    destination_path = os.path.join(ml_lib_path, ml_lib_name_new)
    print(f"Copying library file from: {source_path} to: {destination_path}")
    
    if os.path.exists(source_path):
        # Copy the library file to the correct location and rename it with lib prefix
        shutil.copy(source_path, destination_path)
        print(f"File copied and renamed successfully for directory: {dir_name}")
    else:
        print(f"Source file does not exist for directory: {dir_name}")

print("ML fix action completed")
