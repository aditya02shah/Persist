import subprocess
import difflib

exec_name = "./persist"

def normalize(s):
    return s.replace("\r\n", "\n").strip()

def run_test(testname: str, truthfile: str, verbose: bool = False):
    """
    Runs <testname> and compares output against content of <truthfile>

    """
    print(f"Running Test - {testname} ..")
    # run test and store output
    with open(testname, "r") as file:
        result = subprocess.run([exec_name], stdin = file, capture_output = True, text = True)

    output = result.stdout

    if verbose:
        print("Test output is: \n", output)

    # read expected output
    with open(truthfile, "r") as tfile:
        expected_output = tfile.read()

    if verbose:
        print("Expected output is: \n", output)

    # normalize output
    output_norm = normalize(output)
    expected_norm = normalize(expected_output)

    # compare output with truthfile
    if output_norm == expected_norm:
        print(f"{testname} passed!")
    else:
        print(f"{testname} failed!")
        # display diff
        print("Diff:")
        diff = difflib.unified_diff(
            expected_norm.splitlines(),
            output_norm.splitlines(),
            fromfile="expected",
            tofile="actual",
            lineterm=""
        )
        print("\n".join(diff))

run_test("tests/test_0.txt", "tests/expected_0.txt")