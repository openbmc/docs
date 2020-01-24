# PLDM File I/O
PLDM for file based I/O defines data structures and commands to read/write files between two PLDM termini. Before accessing any files, a PLDM requester must obtain information about the set of files that are currently present with the PLDM responder. The information about the files contains well-known file names, file sizes and file traits. Such information is contained in tables, and these tables may be updated. Once a requester is aware of metadata of files such file name, file size and file handle commands to read/write the file may be issued. For PLDM file I/O, the File Attribute Table comprises of metadata for files. The file attribute table is initialised by parsing the config file containing information about the files.

'OEM_JSONS_DIR'('')  - is where the json config file is stored. The json file is part of the image and get placed there. PLDM reads the config files at runtime and builds the file tables and places at OEM_TABLES_DIR ('').
Config file currently prsent is:
  - config.json

The config json file needs to be defined for a new system and should be placed at "" in the bmc.

Any new json files should be added here:  [pldm-recipe](https://github.com/openbmc/openbmc/blob/master/meta-ibm/meta-witherspoon/recipes-phosphor/pldm/pldm_%25.bbappend) or in the system specific recipe.
