# Automated Documentation Augmentation

OpenBMC is comprised of many repositories.  Each repository has some
documentation in the form of doxygen in the code and also some explicitly
defined documentation in its corresponding bitbake recipe.

To simplify introduction to developing in OpenBMC, the documentation in these
two places will be combined automatically.

Each repository will now provide a json file that will be used to identify the
recipe to parse, and provide additional information.

The following fields are required:

| Field | Type | Detail |
| ---   | ---- | ------ |
| `recipe` | string | recipe name, e.g. `phosphor-host-ipmid` |
| `categories` | list of strings | list of categories for this code |


The following fields are optional, and when used override what's in the bitbake recipe:

| Field | Type | Detail |
| ----- | ---- | ------ |
| `summary` | string | a short summary of this code's purpose |
| `description` | string | a full description of this code's purpose and interactions, expectations, etc |
| `depends` | list of strings | list of dependencies

The information will be collated and used as a landing page to jump into the
doxygen for each repository.

Example:

```
{
  "recipe": "phosphor-hwmon",
  "categories": ["hwmon", "dbus", "sensors"],
  "summary": "Daemon that reads hwmon sensors and reports values to dbus."
}
```
