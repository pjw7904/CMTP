import os


def control_overhead_helper(file_name):
    f = open(file_name,"r")
   
    line = f.readline()
    
    size = 0
    while line:
        if "FAILURE UPDATE message received" in line:
            line = f.readline()
            size += int(line.split("=")[1])
        line = f.readline()   
    
    f.close()
    return size

def calculate_control_overhead():
    log_directory = os.getcwd()+"\\logs"
    files = os.listdir(log_directory)

    total_size = 0

    for file in files:
        if os.path.getsize(log_directory+"\\"+file) > 0:
            total_size += control_overhead_helper(log_directory+"\\"+file)
    return total_size

def convergent_time_helper(file_name):
    f = open(file_name,"r")

    line = f.readline()

    while line:
        if "Detected a failure" in line:
            token = line.split(" ")
            return 0,int(token[len(token)-1])
        elif "FAILURE UPDATE message received" in line:
            token = line.split(" ")
            return 1,int(token[6].replace(",",""))
        line = f.readline()
    return -1,-1

def calculate_convergent_time():
    log_directory = os.getcwd()+"\\logs"
    begin_time = -1
    end_time = -1
    log_directory = os.getcwd()+"\\logs"
    files = os.listdir(log_directory)

    for file in files:
        if os.path.getsize(log_directory+"\\"+file) > 0:
            type,value = convergent_time_helper(log_directory+"\\"+file)
            if type == 0:
                begin_time = value
            else:
                end_time = max(end_time,value)
    return end_time - begin_time

def number_of_change_helper(file_name):
    f = open(file_name,"r")

    line = f.readline()

    while line:
        if "Detected a failure" in line or "FAILURE UPDATE message received" in line:
            return 1
        line = f.readline()
    return 0


def calculate_number_of_change():
    log_directory = os.getcwd()+"\\logs"
    change_count = 0
    log_directory = os.getcwd()+"\\logs"
    files = os.listdir(log_directory)

    for file in files:
        if os.path.getsize(log_directory+"\\"+file) > 0:
            change_count += number_of_change_helper(log_directory+"\\"+file)
    return change_count

def main():
    print("1.Control overhead")
    print("2.Convergent time")
    print("3.Number of change")
    option = int(input("Enter your option (1-3): "))
    print()
    if option == 1:
        print("Control overhead (byte) =",calculate_control_overhead())
    elif option == 2:
        print("Convergent time (ms) =",calculate_convergent_time())
    else:
        print("Number of change =",calculate_number_of_change())
    
    
if __name__ == "__main__":
    main()