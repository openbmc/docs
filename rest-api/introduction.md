# OpenBMC REST API

The primary management interface for OpenBMC is REST. This document provides
some basic structure and usage examples for the REST interface.

The schema for the rest interface is directly defined by the OpenBMC D-Bus
structure. Therefore, the objects, attributes and methods closely map to those
in the D-Bus schema.

For a quick explanation of HTTP verbs and how they relate to a RESTful API, see
<http://www.restapitutorial.com/lessons/httpmethods.html>.

## Logging in

Before you can do anything you first need to log in:

    curl -c cjar -k -X POST -H "Content-Type: application/json" \
        -d '{"data": [ "root", "0penBmc" ] }' \
        https://bmc/login


This performs a login using the provided username and password, and stores the
newly-acquired authentication credentials in the `curl` "Cookie jar" file (named
`cjar` in this example). We will use this file (with the `-b` argument) for
future requests.

To log out:

    curl -c cjar -b cjar -k -X POST -H "Content-Type: application/json" \
        -d '{"data": [ ] }' \
	https://bmc/logout

(or just delete your cookie jar file)

