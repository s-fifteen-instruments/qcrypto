from S15lib.g2lib import g2lib
import os
import struct
import numpy as np
import matplotlib.pyplot as plt
import contextlib

raw_time_list = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/rawevents/raw_time_list"
pfind_list = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/processed/pfind_time_diff_list"

#test data with 3 s of data
file1 = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/rawevents/a"
file2 = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/rawevents/b"
delay = -4185824

raw_dir = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/rawevents/"
dirPath = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/plots/"
if not os.path.isdir(dirPath):
    os.mkdir(dirPath)


@contextlib.contextmanager
def my_open(file_name: str):
     f = os.open(file_name, os.O_RDONLY)
     fd = os.fdopen(f)
     try:
         yield fd
     finally:
         os.close(f)

def get_list(file_name: str):
    with my_open(file_name) as f:
        #lines = list(f)
        lines = [x.split() for x in f]
    return lines

def ch_to_patt(channel):
    return int(2 ** (channel - 1))


class RawTs(object):
    """
    Class containing the two time sequences, and stuff
    """

    def __init__(self, file1 = file1, file2 = file2, delay = delay, bins: int = 128, bin_width : float = 0.5):
        """
        Loads data from file
        """
        resolution = 256 # 0.5,8 or 256
        swap = True
        self.fname = '/tmp/tmp'
        self.ts_list1 = []
        self.ts_list2 = []
        self.event_channel_list1 = []
        self.event_channel_list2 = []
        self.read_ts(file1, resolution, swap, self.ts_list1, self.event_channel_list1)
        self.read_ts(file2, resolution, swap, self.ts_list2, self.event_channel_list2)
        self.t1 = np.array(self.ts_list1) / resolution
        self.t2 = np.array(self.ts_list2) / resolution
        self.delay_time = delay
        self.bins = bins
        self.ch_stop_delay = self.delay_time/8 + (bins/2*bin_width) #delay in nsecs and shift to centre of histogram
        self.bin_width = bin_width
        self.histo = g2lib.delta_loop(self.t1, self.t2 + self.ch_stop_delay,
                                 bins = bins,
                                 bin_width_ns = bin_width
                                 )
        self.x = np.arange(bins)*self.bin_width - (bins/2)*self.bin_width

    def g2_chans(self, ch1, ch2, bin_width = 0.125):
        resolution = 256
        t_ch0 = self.t1[[int(ch,2) & ch_to_patt(ch1) != 0 for ch in self.event_channel_list1]]
        t_ch1 = self.t2[[int(ch,2) & ch_to_patt(ch2) != 0 for ch in self.event_channel_list2]]
        self.ch_stop_delay = self.delay_time/8 + (self.bins/2*bin_width) #delay in nsecs and shift to centre of histogram
        self.histo = g2lib.delta_loop(t_ch0, t_ch1 + self.ch_stop_delay,
                                 bins = self.bins,
                                 bin_width_ns = bin_width
                                 )
        self.x = np.arange(self.bins)*bin_width - (self.bins/2)*bin_width

    def plot_hist_show(self):
        plt.xlabel('delay (ns)')
        plt.ylabel('counts')
        plt.plot(self.x,self.histo)
        plt.legend(['Delay : ' + str(self.ch_stop_delay)])
        plt.show()

    def plot_hist(self):
        plt.xlabel('delay (ns)')
        plt.ylabel('counts')
        plt.plot(self.x,self.histo)
        plt.legend(['Delay : ' + str(self.ch_stop_delay)])
        plt.savefig(self.fname + '.png')
        plt.savefig(self.fname + '.svg')
        plt.close()


    def read_ts(self, filename, resolution, swap,ts_list, event_ch_list):   
        if resolution == 256 : # 8 ps timestamp
           shift_val = 10 # (64-10)= 54 bits of time info
        elif resolution == 0.5 : # 2 ns timestamp
           shift_val = 19 #  45 bits of time info
        elif resolution == 8 :
           shift_val = 15 #  49 bits of time info
        with open(filename, 'rb') as f:
            byte8 = f.read(8)
            while byte8 != b"":
                if swap == True:
                    byte4a = byte8[0:4]
                    byte4b = byte8[4:8]
                    byte8 = byte4b + byte4a
                value, = struct.unpack('<Q',byte8)
                pattern = value & 0x1F #byte8 & b"0x1F"
                temp =  value >> shift_val # 49 bits of time info
                time_stamp = temp #.to_bytes(8,'big')
                if (pattern & 0x10) == 0:
                    ts_list.append(time_stamp)
                    event_ch_list.append("{0:04b}".format(pattern & 0xF))
                byte8 = f.read(8)

def process_folder():
    t_list = get_list(raw_time_list)
    t_list = [val for sublist in t_list for val in sublist]
    delay_list = get_list(pfind_list)
    delay_list = [val[1] for val in delay_list]

    comb_list = list(zip(t_list,delay_list))

    for ts, delay in comb_list:
        filename1 = raw_dir + "raw_a_" + ts
        filename2 = raw_dir + "raw_b_" + ts
        a = RawTs(file1 = filename1, file2 = filename2, delay = int(delay))
        a.fname = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/plots/" + ts[-6:] # base filename will be day and hour
        a.plot_hist()

def test_single():
    file1 = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/rawevents/raw_a_202208170946"
    file2 = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/rawevents/raw_b_202208170946"
    #delay = 10720560
    #file1 = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/rawevents/st_a_"
    #file2 = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/rawevents/st_b_"
    delay = -72922784 
    file1 = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/rawevents/raw_a_202208171030_short"
    file2 = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/rawevents/raw_b_202208171030_short"
    delay = -56949136
    #file1 = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/rawevents/a"
    #file2 = "/home/s-fifteen/programs/qcrypto/remotecrypto/data/rawevents/b"
    #delay = 1180224
    a = RawTs(file1 = file1, file2 = file2, delay = int(delay), bins = 60, bin_width=0.125)
    a.fname = 'plots/test'
    a.plot_hist_show()
    return a

def single():
    f = open('rawevents/raw_time_list')
    date_list = f.readlines()
    f.close()
    single_date = date_list[-1].strip()
    file1 = os.getcwd() + '/rawevents/raw_a_' + single_date
    file2 = os.getcwd() + '/rawevents/raw_b_' + single_date
    f = open('processed_test/single_pfind')
    data = f.readline().strip().split()
    delay = data[1]
    a = RawTs(file1 = file1, file2 = file2, delay = int(delay), bins = 120, bin_width=0.125)
    a.fname = 'plots/test'
    directory = a.fname.split('/')[0]
    if not os.path.exists(directory):
        os.mkdir(directory)
    a.plot_hist()
    return a

if __name__ == '__main__':
    #process_folder()
    a = single()
