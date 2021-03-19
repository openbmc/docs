# OpenBMC Web User Interface Development

**Document Purpose:** How to customize, build, and run the OpenBMC Web UI

**Audience:** Developer familiar with HTML, JS, and Vue.js

**Prerequisites:** Current Linux, Mac, or Windows system

## Getting started

The [webui-vue](https://github.com/openbmc/webui-vue) repository
provides a web-based interface for OpenBMC. The OpenBMC Web UI uses the
[Vue.js](https://vuejs.org/) framework to interact with the BMC via the
Redfish API. It allows users to view hardware information, update firmware,
set network settings, and much more.

Visit [CONTRIBUTING.md](https://github.com/openbmc/webui-vue/blob/master/CONTRIBUTING.md)
to find information on:
- Project Setup
- Code of Conduct
- Asking Questions
- Submitting Bugs
- Requesting Features
- User Research
- Design Reviews
- Help Wanted
- Code Reviews

Visit the [OpenBMC Web UI Style Guide](https://openbmc.github.io/webui-vue/)
to find information on:
- Coding Standards
- Guidelines
- Unit Testing
- Components Usage
- Quick start references for building new pages

Visit the [OpenBMC Themes Guide - How to customize](https://openbmc.github.io/webui-vue/themes/customize.html)
to learn how to create custom builds to meet your branding and customization
needs for:
- Routing
- Navigation
- State Store
- Theming


## Load web UI against QEMU

Connect to web UI in QEMU

1. You will need the QEMU session running per instructions in the
"Download and Start QEMU Session" section of
[dev-environment](https://github.com/openbmc/docs/blob/master/development/dev-environment.md).

2. Assuming you used the default of 2443 for the HTTPS port in your QEMU
session, you will point your web browser to https://localhost:2443.

3. Login with default username and password and verify basic Web UI features are
working as expected.

**Note** You will need to approve the security exception in your browser to
connect. OpenBMC is running with a self-signed SSL certificate. Accepting
this is key for the next steps.