import os
Import("env")

print("Running ML fix action")

# Define the base path to check all subdirectories
base_path = os.path.join(env.subst("$PROJECT_DIR"), ".pio/libdeps/esp32dev/ML_SynthTools/src/")

# Walk through all subdirectories and files
for root, dirs, files in os.walk(base_path):
    for file_name in files:
        if file_name.endswith(".a") and not file_name.startswith("lib"):
            # Construct full paths for the source and destination
            source_path = os.path.join(root, file_name)
            destination_path = os.path.join(root, "lib" + file_name)
            
            # Print status
            print(f"Renaming file from: {source_path} to: {destination_path}")
            
            # Rename the file with "lib" prefix
            os.rename(source_path, destination_path)
            print(f"File renamed successfully: {destination_path}")

print("ML fix action completed")
