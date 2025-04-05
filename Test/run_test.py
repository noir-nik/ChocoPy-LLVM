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

def main():
    parser = argparse.ArgumentParser(description="Run ChocoPy tests.")
    parser.add_argument('-e', dest='executable', help='Specify the ChocoPy executable', default=CHOCOPY_LLVM_EXECUTABLE)
    parser.add_argument('--dump', action='store_true', help='Dump the AST')
    parser.add_argument('--dump_dir', help='Dump expected and actual AST of failed tests to files in the given directory')
    parser.add_argument('--dump_only', action='store_true', help='Only dump the AST')
    parser.add_argument('--verbose', type=int, help='Verbose output', default=0, nargs='?')

    args, TEST_FILES = parser.parse_known_args()

    executable = args.executable

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

    if not args.dump_only:
        print(f"Running {len(all_files)} test" + ("s" if len(all_files) > 1 else ""))
    # print(all_files)

    test_count = len(all_files)
    passed = 0

    for file in all_files:
        ast_file = file + ".ast"
        if not os.path.isfile(file) or not os.path.isfile(ast_file):
            test_count -= 1
            if (args.verbose > 1):
                print(f"No ast file for {file}")
            continue
        
        result = subprocess.run([executable, file, "-ast-dump"],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                check=True)
        # print(expected)
        decoded = result.stdout.decode()
        if args.dump_only:
            sys.stdout.write(decoded)
            exit(0)
        decoded_stderr = result.stderr.decode()
        lines_decoded = decoded.splitlines(keepends=True)
        with open(ast_file) as f:
            ast_file_content = f.read()
            lines_expected = ast_file_content.splitlines(keepends=True)
        if len(lines_expected) != len(lines_decoded):
            if (args.verbose > 1):
                print(f"Error in {file}")
                print(f"Expected {len(lines_expected)+1} lines, got {len(lines_decoded)}")
            # sys.exit(1)
        success = False
        def dump():
            print("Expected AST:")
            print(ast_file_content)
            print("Got AST:")
            sys.stderr.write(decoded)
        if (args.dump):
            dump()
        for i, (expected_line, decoded_line) in enumerate(zip(lines_expected, lines_decoded)):
            if expected_line.strip() != decoded_line.strip():
                success = False
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
            success = True
        if args.dump_dir:
            if not success:
                dump_dir(args.dump_dir, file, ast_file_content, decoded)

        if success:
            if (args.verbose > 0):
                print(f"Pass: {file}")
            passed += 1
        else:
            if (args.verbose > 0):
                print(f"Fail: {file}")
                # dump()

    print(f"{passed}/{test_count} tests passed")

if __name__ == "__main__":
    main()

