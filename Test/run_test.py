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
    cmd = [args.executable, file, "--ast-dump"] + args.flags
    try:
        result = subprocess.run(cmd,
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

            # dump()
    return is_success

def on_err(file: str, args, flags: str = "")-> bool:
    cmd = [args.executable, file, flags] + args.flags
    if (args.verbose > 1):
        print(f"Command: {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd,
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
    lines_test = [line for line in lines_expected]
    lines_actual = decoded_stderr.splitlines(keepends=True)

    def get_next_output_line():
        for line in lines_actual:
                yield line

    output_line_gen = get_next_output_line()

    is_success = True
    for i, line in enumerate(lines_expected, start=1): 
        if line.startswith("CHECK-NEXT:"):
            line_to_check = line[len("CHECK-NEXT:"):]
            is_strictly_next = True
        elif line.startswith("CHECK:"):
            line_to_check = line[len("CHECK:"):]
            is_strictly_next = False
        else:
            continue

        current_output_line = next(output_line_gen, None)

        check_stripped = line_to_check.strip()
        while current_output_line is not None and check_stripped != current_output_line.strip():
            if is_strictly_next:
                is_success = False
                if (args.verbose > 1):
                    print(f"Error in {file}.err:{i}")
                    print(f"Expected: {line_to_check.strip()}")
                    print(f"Got: {current_output_line}")
                break
            current_output_line = next(output_line_gen, None)
        if current_output_line is None:
            if (args.verbose > 1):
                print(f"Error: Expected line not found: {line_to_check.strip()}")
            is_success =  False
            break

        if not is_success:
            continue
    if (not is_success and args.verbose > 1):
        print(f"Command: {' '.join(cmd)}")
        print(f"Output:")
        print(decoded_stderr)

    return is_success
 

def main():
    parser = argparse.ArgumentParser(description="Run ChocoPy tests.")
    parser.add_argument('-e', dest='executable', help='Specify the ChocoPy executable', default=CHOCOPY_LLVM_EXECUTABLE)
    parser.add_argument('--dump', action='store_true', help='Dump the AST')
    parser.add_argument('--dump_dir', help='Dump expected and actual AST of failed tests to files in the given directory')
    parser.add_argument('--dump_only', action='store_true', help='Only dump the AST')
    parser.add_argument('-f', '--flags', dest='flags', nargs='+', help='Specify additional flags to pass to the ChocoPy executable', default=[])
    parser.add_argument('-v', '--verbose', action='store_const',  const=1, default=0, help='Increase verbosity')
    parser.add_argument('-vv', '--very-verbose', action='store_const', const=2, default=0, help='Increase verbosity even more')


    args, TEST_FILES = parser.parse_known_args()

    verbosity = args.verbose if args.verbose > 0 else args.very_verbose
    args.verbose = verbosity

    if not os.path.isfile((args.executable)):
        print(f"ChocoPy executable {args.executable} not found, specify correct path to chocopy-llvm")
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
        ast_file = file + ".ast"
        err_file = file + ".err"
        if os.path.isfile(ast_file):
            is_success = on_ast(file, args)
        else:
            is_success = on_err(file, args)
            
        if is_success:
            passed += 1
            if (args.verbose > 0):
                print(f"Pass: {file}")
        else:
            if (args.verbose > 0):
                print(f"Fail: {file}")
 
       

    print(f"{passed}/{test_count} tests passed")

if __name__ == "__main__":
    main()

