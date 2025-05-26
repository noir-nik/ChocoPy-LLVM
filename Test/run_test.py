import subprocess
import sys
import glob
import os
import argparse
import shutil

CHOCOPY_LLVM_EXECUTABLE = "chocopy-llvm"

def dump_dir(dir: str, input_file: str, expected: str, actual: str):
    # create dir if it doesn't exist
    if not os.path.isdir(dir):
        os.mkdir(dir)
        
    # copy input_file to dir
    name = os.path.basename(input_file)
    shutil.copyfile(input_file, os.path.join(dir, name))
    
    # write expected and actual to dir
    with open(os.path.join(dir, name + ".expected.json"), 'w') as f:
        f.write(expected)
    with open(os.path.join(dir, name + ".actual.json"), 'w') as f:
        f.write(actual)

def on_ast(file: str, args)-> bool:
    try:
        result = subprocess.run([args.executable, file, "--ast-dump"],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                check=True)
    except subprocess.CalledProcessError as e:
        print(f"Failed to run {file}")
        return False
    # print(expected)
    decoded = result.stdout.decode()
    if args.dump_only:
        sys.stdout.write(decoded)
        exit(0)
    decoded_stderr = result.stderr.decode()
    lines_decoded = decoded.splitlines(keepends=True)
    ast_file = file + ".ast"
    with open(ast_file) as f:
        ast_file_content = f.read()
        lines_expected = ast_file_content.splitlines(keepends=True)
    if len(lines_expected) != len(lines_decoded):
        if (args.verbose > 1):
            print(f"Error in {file}")
            print(f"Expected {len(lines_expected)+1} lines, got {len(lines_decoded)}")
        # sys.exit(1)
    is_success = False
    def dump():
        print("Expected AST:")
        print(ast_file_content)
        print("Got AST:")
        sys.stderr.write(decoded)
    if (args.dump):
        dump()
    for i, (expected_line, decoded_line) in enumerate(zip(lines_expected, lines_decoded)):
        if expected_line.strip() != decoded_line.strip():
            is_success = False
            if (args.verbose > 1):
                # sys.stderr.write(decoded_stderr)
                print(decoded_stderr)
                print(f"Line {i} does not match for {file}")
                err_print_range = 5
                start = max(i - err_print_range, 0)
                end = i + err_print_range
                print("Expected:")
                get_arrow = lambda x: "->" if x == i else "  "
                for j in range(start, min(end, len(lines_expected))):
                    print(f"{get_arrow(j):<2}{j:>4}", lines_expected[j], end="")
                print("Got:")
                for j in range(start, min(end, len(lines_decoded))):
                    print(f"{get_arrow(j):<2}{j:>4}", lines_decoded[j], end="")
                print()
            break
        is_success = True
    if args.dump_dir:
        if not is_success:
            dump_dir(args.dump_dir, file, ast_file_content, decoded)

    if is_success:
        if (args.verbose > 0):
            print(f"Pass: {file}")
    else:
        if (args.verbose > 0):
            print(f"Fail: {file}")
            # dump()
    return is_success

def on_err(file: str, args, flags: str = "")-> bool:
    try:
        result = subprocess.run([args.executable, file, flags],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                check=True)
    except subprocess.CalledProcessError as e:
        print(f"Failed to run {file}")
        return False
    decoded = result.stdout.decode()
    decoded_stderr = result.stderr.decode()
    if args.dump_only:
        sys.stdout.write(decoded)
        exit(0)
    with open(file + ".err") as f:
        lines_expected = f.readlines()
    lines_expected = [line for line in lines_expected if line.startswith("CHECK:")]
    lines_actual = decoded_stderr.splitlines(keepends=True)

    is_success = True
    for i, (expected_line, actual_line) in enumerate(zip(lines_expected, lines_actual)):
        if not actual_line.startswith(expected_line[6:]):
            is_success = False
            if (args.verbose > 1):
                print(f"Line {i} does not match for {file}")
                err_print_range = 5
                start = max(i - err_print_range, 0)
                end = i + err_print_range
                print("Expected:")
                get_arrow = lambda x: "->" if x == i else "  "
                for j in range(start, min(end, len(lines_expected))):
                    print(f"{get_arrow(j):<2}{j:>4}", lines_expected[j], end="")
                print("Got:")
                for j in range(start, min(end, len(lines_actual))):
                    print(f"{get_arrow(j):<2}{j:>4}", lines_actual[j], end="")
                print()
            break
    return False


def main():
    parser = argparse.ArgumentParser(description="Run ChocoPy tests.")
    parser.add_argument('-e', dest='executable', help='Specify the ChocoPy executable', default=CHOCOPY_LLVM_EXECUTABLE)
    parser.add_argument('--dump', action='store_true', help='Dump the AST')
    parser.add_argument('--dump_dir', help='Dump expected and actual AST of failed tests to files in the given directory')
    parser.add_argument('--dump_only', action='store_true', help='Only dump the AST')
    parser.add_argument('--verbose', type=int, help='Verbose output', default=0, nargs='?')

    args, TEST_FILES = parser.parse_known_args()

    executable = args.executable

    if not os.path.isfile((executable)):
        print(f"ChocoPy executable {executable} not found, specify correct path to chocopy-llvm")
        parser.print_usage()
        sys.exit(1)

    # Collect test files
    if not TEST_FILES:
        print("No test files specified.")
        parser.print_usage()
        sys.exit(1)

    all_files = []
    for file in TEST_FILES:
        if os.path.isdir(file):
            all_files.extend(glob.glob(file + "/*.py"))
        else:
            all_files.append(file)


    for file in all_files:
        ast_file = file + ".ast"
        err_file = file + ".err"
        if not os.path.isfile(file) or not (os.path.isfile(ast_file) or os.path.isfile(err_file)):
            all_files.remove(file)
            if (args.verbose > 1):
                print(f"No ast or err file for {file}")
            continue
    
    test_count = len(all_files)
    passed = 0
    

    if not args.dump_only:
        print(f"Running {test_count} test" + ("s" if test_count > 1 else ""))

    for file in all_files:
        if os.path.isfile(ast_file):
            if on_ast(file, args):
                passed += 1
        else:
            if on_err(file, args):
                passed += 1
 
       

    print(f"{passed}/{test_count} tests passed")

if __name__ == "__main__":
    main()

