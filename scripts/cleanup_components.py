import os
import subprocess
import shutil
import fnmatch

# Path of the current script
script_path = os.path.abspath(__file__)

# Directory of the script
script_dir = os.path.dirname(script_path)

# Parent directory of the script
bist_dir = os.path.dirname(script_dir)

def get_dependency_files(build_dir, dep_path):
    """ Extract dependency files from ninja dependency tool output. """
    dep_files = set()
    try:
        # Get dependencies from ninja
        result = subprocess.run(['ninja', '-C', build_dir, '-t', 'deps'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        for line in result.stdout.split('\n'):
            if dep_path in line:
                path = line.strip().split(':')[0].strip()
                if path.startswith(dep_path):
                    # Calculate the absolute path and normalize it
                    absolute_path = os.path.normpath(os.path.join(build_dir, path))
                    dep_files.add(absolute_path)
    except subprocess.CalledProcessError as e:
        print(f"Failed to get dependencies: {e}")
    return dep_files


def compile_project(test_dir, soc_target):
    """ Run cmake and ninja to compile the project. """
    build_dir = os.path.join(test_dir, 'build')
    os.makedirs(build_dir, exist_ok=True)
    try:
        # Run CMake and Ninja build
        subprocess.run(['cmake', f'-DSOC_TARGET={soc_target}', '-B', 'build', '-GNinja'], cwd=test_dir, check=True)
        subprocess.run(['ninja', '-C', 'build'], cwd=test_dir, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Compilation failed: {e}")
        exit(1)
    return build_dir

def cleanup_build_directory(build_dir):
    """ Remove the build directory after use. """
    try:
        shutil.rmtree(build_dir)
        print(f"Removed build directory: {build_dir}")
    except OSError as e:
        print(f"Error removing build directory {build_dir}: {e}")

def delete_unused_files_and_empty_dirs(components_dir, all_deps, exclusions):
    for root, dirs, files in os.walk(components_dir, topdown=False):
        for file in files:
            file_path = os.path.join(root, file)
            if file_path not in all_deps and not any(fnmatch.fnmatch(file_path, pattern) for pattern in exclusions):
                os.remove(file_path)

        if not os.listdir(root):
            os.rmdir(root)

def main():
    soc_target = 'esp32c3'  # Change as needed
    tests_dir = f'{bist_dir}/tests'
    components_dir = f'{bist_dir}/components'

    # Files or patterns to exclude from deletion
    exclusions = [
    ]

    all_deps = set()

    # Compile projects and gather dependencies
    for test_subdir in os.listdir(tests_dir):
        print(f"Processing test: {test_subdir}")
        test_path = os.path.join(tests_dir, test_subdir)
        if os.path.isdir(test_path):
            build_dir = compile_project(test_path, soc_target)
            deps = get_dependency_files(build_dir, components_dir)
            all_deps.update(deps)
            cleanup_build_directory(build_dir)

    # Clean up components directory
    delete_unused_files_and_empty_dirs(components_dir, all_deps, exclusions)

    print("Cleanup completed")

if __name__ == "__main__":
    main()
