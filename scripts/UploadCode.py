'''
Author: Peter Willis (pjw7904@rit.edu)
Last Updated: 05/18/2021
Desc: Automated configuration of remote nodes on GENI
'''

from GENIutils import uploadToGENINode, getConfigInfo, buildDictonary

def main():
  # Grabbing configuration info from GENI config file
  rspec = getConfigInfo("Local Utilities", "RSPEC")
  GENIDict = buildDictonary(rspec)
  codeSource = getConfigInfo("MTP Utilities", "localCodeDirectory")
  codeDestination = getConfigInfo("GENI Credentials", "remoteCodeDirectory")

  print("\n+---------Number of Nodes: {0}--------+".format(len(GENIDict)))
  for node in GENIDict:
    if node[0] != 'C':
        uploadToGENINode(node, GENIDict, codeSource, codeDestination)
        print("{0} - Upload complete".format(node))

if __name__ == "__main__":
    main() # run main
