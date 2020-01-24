# PLDM BIOS
The BIOS code is implemented following PLDM specification DSP0247. The BIOS attributes are owned/maintained by bmc (not like other traditional cases where bios attributes are maintained by the host) but are applicable for Host in most of the cases. Currently string, enum and integer type BIOS attributes are supported among all the possible BIOS attributes. Each type has it's own json config file depicting the attribute names, dbus paths and other mandatory parameters. 
'BIOS_JSONS_DIR'('/usr/share/pldm/bios')  - is where the json config files are stored. The json files are part of the image and get placed there. PLDM reads the config files at runtime and builds the bios tables and places at BIOS_TABLES_DIR ('/var/lib/pldm/bios').
Following three config files are present currently:
  - enum_attrs.json
  - integer_attrs.json
  - string_attrs.json

All these BIOS json files need to be defined for a new system and should be placed at "/usr/share/pldm/bios" in the bmc. Example json files can be found at  [pldm-bios](https://github.com/openbmc/pldm/tree/master/libpldmresponder/examples/bios) for the supported types.
Any new json files should be added here:  [pldm-recipe](https://github.com/openbmc/openbmc/blob/master/meta-ibm/meta-witherspoon/recipes-phosphor/pldm/pldm_%25.bbappend) or in the system specific recipe.
