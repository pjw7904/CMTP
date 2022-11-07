from GENIutils import downloadFromGENI, getConfigInfo, buildDictonary
import os

def download_log(node,GENIDict,userRoot):
    log_directory = os.getcwd()+"\\"+"logs"

    if not os.path.exists(log_directory):
        os.makedirs(log_directory)

    print("Downloading log file of node {}".format(node))
    downloadFromGENI(node,GENIDict,"{0}/SRC/MTP_{1}.log".format(userRoot,node),log_directory+"\\"+node+".log")
    print("Download done\n")


def main():
    rspec = getConfigInfo("Local Utilities", "RSPEC")
    GENIDict = buildDictonary(rspec)
    userRoot = getConfigInfo("GENI Credentials","remoteCodeDirectory")

    for node in sorted(GENIDict):
        download_log(node,GENIDict,userRoot)
    
    print("Program done!")



if __name__ == "__main__":
    main()
