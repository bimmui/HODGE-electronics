import pytest
from pytest_embedded import Dut

@pytest.mark.esp32
@pytest.mark.parametrize('port', ['/dev/tty.usbserial-0001'])  # Change for Windows COM port
def test_temperature(dut: Dut):
    """ Test BMP581 Temperature Readings """
    dut.expect_exact("BMP581 Test Ready") 
    temp_c = float(dut.expect(r'Temperature: (-?\d+\.\d+) C')[1]) 
    assert 15.0 <= temp_c <= 30.0

def test_pressure(dut: Dut):
    """ Test BMP581 Pressure Readings """
    pressure = float(dut.expect(r'Pressure: (\d+\.\d+) Pa')[1])
    assert 98000.0 <= pressure <= 103500.0

def test_altitude(dut: Dut):
    """ Test BMP581 Altitude Calculation """
    altitude = float(dut.expect(r'Altitude: (-?\d+\.\d+) m')[1]) 
    assert -10.0 <= altitude <= 100.0