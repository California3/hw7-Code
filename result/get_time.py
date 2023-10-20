g2 = ["40", "80"]
g2_p1 = ["1", "2", "4", "8", "16", "32", "64", "128"]


g1 = ["16", "64"]
g1_p2 = ["0","10","20","30","40","50","60","70","80","90","100"]

txt_loc = "Task4_Result"
txt_file_end = "-binary.txt"

def get_time_list(time, rrlist):
    for rr in rrlist:
        txt_path = txt_loc + "/" + rr + "-" + time + txt_file_end
        print(fetch_time(txt_path))

def get_rr_list(rr, timelist):
    for time in timelist:
        txt_path = txt_loc + "/" + rr + "-" + time + txt_file_end
        print(fetch_time(txt_path))

def fetch_time(path):
    with open(path, 'r') as f:
        lines = f.readlines()
    return lines[-1].split(": ")[1].split(" seconds")[0]

get_time_list(g2[0], g2_p1)
get_rr_list(g1[0], g1_p2)