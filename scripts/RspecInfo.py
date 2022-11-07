from GENIutils import getConfigInfo, buildDictonary, printDictonaryContent,orchestrateRemoteCommands
import re

def searchPort(node,GENIDict):
    output = orchestrateRemoteCommands(node,GENIDict,"ifconfig",True).decode().strip().split("\n")
    output_dict = {}
    i = 0
    while i < len(output):
        if re.search(r'eth\d+:\sflag',output[i]):
            output_dict[output[i+1].strip().split(" ")[1].strip()] = output[i].split(":")[0].strip()
            i += 1
        i += 1

    return output_dict


def main():
    # Grabbing configuration info from GENI config file
    rspec = getConfigInfo("Local Utilities", "RSPEC")
    GENIDict = buildDictonary(rspec)


    option = input("Include port? (yes/no): ")

    if len(option) == 0:
        printDictonaryContent(GENIDict)
    elif option[0] == 'n':
        printDictonaryContent(GENIDict)
    elif option[0] == 'y':
        printDictonaryContent(GENIDict,searchPort)
    else:
        print("Program ended")

if __name__ == "__main__":
    main() # run main