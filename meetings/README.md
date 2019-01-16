OpenBMC Meetings
================

This takes the YAML formatted `meetings.yaml` file and converts to a `.ics`
file that can be used as input into a calendar. This process allows for adding
additional meetings in the future, just by adding another record to the yaml
file, as well as automating ical generation and email. The tool that is used to
do the conversion is `yaml2ical`. Follow the usage and installation
instructions here:

https://github.com/openstack-infra/yaml2ical

Start by editing `meetings.yaml`, changing the fields to the needed values. You
can get more description on the supported fields at the link above.

Once you are happy with the info in the yaml file, run the tool `yaml2ical`:

```
$ yaml2ical -f -y . -o meetings.ics
```

The `-f` parameter forces an overwrite of the existing ical file.
