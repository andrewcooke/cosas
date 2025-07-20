#!/usr/bin/python3

from os.path import exists
from os import remove
from functools import cache

# guessed from plot (within 0.25?)
# +ve values from DNL, -ve from INL
# -ve sample number inferred from pattern (might be ignoring step near 2048)
#DNL_OBS = {512: 9.0, 1536: 7.25, 2048: -3.0, 2560: 7.5, 3072: -1.0, 3584: 8.0}

# but for monotonic conversion DNL cannot be less than -1 so presumably it's more like
DNL_OBS = {512: 9.0, 1536: 7.25, 2047: -1.0, 2048: -1.0, 2049: -1.0, 2560: 7.5, 3072: -1.0, 3584: 8.0}
# (which might be visible in the DNL plot in the data sheet)
# (i'm also ignoring something at 511 in the DNL plot that may be an aliasing issue in the plot itself?)

# i find it easier to think of bin widths with expected value of 1 (these are non-negative)
WID_OBS = {k: v+1 for k, v in DNL_OBS.items()}

# assume others are equal in width and the total width is given
WID_OTHER = (4096 - sum(WID_OBS.values() )) / (4096 - len(DNL_OBS))

# i'm using indices from 0 which may be wrong?

# given an analog value, calculate the measured value by the dac
# from https://www.allaboutcircuits.com/technical-articles/understanding-analog-to-digital-converter-differential-nonlinearity-dnl-error/
# (esp figure 1 and the text below) index 1 corresponds to an output of 1
@cache
def model_resp(a):
    d = 0
    a -= WID_OTHER / 2  # if a is less than 1/2 a typical gap (or 1) then d is 0
    while a > 0:
        d += 1
        # move to next bin
        if d in WID_OBS:
            a -= WID_OBS[d]
        else:
            a -= WID_OTHER
    return d

# plot a function to a file
def plot(fn, path):
    if exists(path): remove(path)
    with open(path, 'w') as out:
        for x in range(4096):
            print(x, fn(x), file=out)
    print(f"output in {path}")

# given a response, calculate the error
def resp_to_err(resp, scale=1):
    if scale != 1: print('scale', scale)
    @cache
    def err(x):
        return resp(x) - x * scale
    return err

# the correction in ComputerCard.h
def cc_correcn(x):
    # uint16_t adc512=ADC_Buffer[cpuPhase][3]+512;
    adc512 = x + 512
    # if (!(adc512 % 0x01FF)) ADC_Buffer[cpuPhase][3] += 4;
    if (adc512 % 0x01ff) == 0: x += 4
    # ADC_Buffer[cpuPhase][3] += (adc512>>10) << 3;
    x += (adc512>>10) << 3
    return x

def apply(resp, correcn):
    def f(x):
        return correcn(resp(x))
    return f
    
plot(model_resp, "/tmp/model_resp")
model_err = resp_to_err(model_resp)
plot(model_err, "/tmp/model_err")

cc_corrected = apply(model_resp, cc_correcn)
plot(cc_corrected, "/tmp/cc_corrected")
cc_err = resp_to_err(cc_corrected)
plot(cc_err, "/tmp/cc_err")
cc_err_scl = resp_to_err(cc_corrected, scale=cc_corrected(4095)/4095)
plot(cc_err_scl, "/tmp/cc_err_scl")

k = 1 << 19

# errors (in model) at 512 1536 (2048) 2560 3584
# diffs 1024 1024 1024
# scale inside 32bit (value is about 12 bits)
def me_correcn(a):
    global k
    b = a + (((a + 0x200) >> 10) << 3)
#    if 512 < a < 2048:
    if (a & 0x600) and not (a & 0x800):
        b += 2
    if (a + 0x200) % 0x400 == 0:
        b -= 4
    return (k * b) >> 19

k = int(4095 * (1 << 19) / me_correcn(4095))
print(k)  # 520222

# useful to check if correcn for cc would be same
k2 = int(4095 * (1 << 19) / cc_correcn(4095))
print(k2)  # 520222

me_corrected = apply(model_resp, me_correcn)
plot(me_corrected, "/tmp/me_corrected")
me_err = resp_to_err(me_corrected)
plot(me_err, "/tmp/me_err")

def cc2_correcn(x):
    adc512 = x + 512
    if (adc512 % 0x01ff) == 0: x += 4
    x += (adc512>>10) << 3
    return (520222 * x) >> 19

cc2_corrected = apply(model_resp, cc2_correcn)
cc2_err = resp_to_err(cc2_corrected)
plot(cc2_err, "/tmp/cc2_err")

