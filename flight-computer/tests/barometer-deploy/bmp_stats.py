import re
import statistics
import pytest
from pytest_embedded_idf import IdfDut

# this is broken btw

EXPECT_TIMEOUT = 60

@pytest.mark.esp32
@pytest.mark.parametrize('port', ['COM7'])
def test_std_calc(dut: IdfDut, port):
    values = []
    debug_pattern = r"Reading:"
    

    try:
        line = dut.expect(debug_pattern, timeout=EXPECT_TIMEOUT)[0]
        print(f"Debug: Found line: {line}")
    except Exception as e:
        debug_output = dut.pexpect_proc.before.decode('utf-8', errors='replace')
        pytest.fail(f"Timeout waiting for debug output. Current buffer: {debug_output}")
    
    full_pattern = r"Reading:\s*([+-]?\d+\.\d{6})"
    
    for _ in range(10):
        try:
            line = dut.expect(full_pattern, timeout=EXPECT_TIMEOUT)[0]
            print(f"Captured line: {line}")
        except Exception as e:
            debug_output = dut.pexpect_proc.before.decode('utf-8', errors='replace')
            pytest.fail(f"Timeout waiting for expected output. Current buffer: {debug_output}")
        match = re.search(full_pattern, line)
        if match:
            values.append(float(match.group(1)))
    
    if values:
        stdev = statistics.stdev(values) if len(values) > 1 else 0.0
        print(f"Collected values: {values}")
        print(f"Standard Deviation: {stdev}")
        assert stdev < 10.0, "Standard deviation is too high"
    else:
        pytest.fail("No valid values were captured.")
