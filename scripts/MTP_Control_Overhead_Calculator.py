import os


def calculate(file_name):
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
    
    

def main():
    log_directory = os.getcwd()+"\\logs"
    files = os.listdir(log_directory)
    
    total_size = 0

    for file in files:
        if os.path.getsize(log_directory+"\\"+file) > 0:
            total_size += calculate(log_directory+"\\"+file)
            
    print("Total size =",total_size)
    
    
if __name__ == "__main__":
    main()