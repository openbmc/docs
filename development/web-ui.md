# OpenBMC Web User Interface Development

**Document Purpose:** How to customize, build, and run the OpenBMC Web UI

**Audience:** Developer familiar with HTML, CSS and JS

**Prerequisites:** Current Linux, Mac, or Windows system

# Webui-vue

The [webui-vue](https://github.com/openbmc/webui-vue) repository will replace
phosphor-webui once it is deprecated. Webui-vue uses the
[Vue.js](https://vuejs.org/) framework to interact with the BMC via the Redfish
API.

Visit [README.md](https://github.com/openbmc/webui-vue/blob/master/README.md) to
learn more about why the Vue.js application was created, features needed to
reach parity and why it is replacing the Angular.JS application.

Visit
[CONTRIBUTING.md](https://github.com/openbmc/webui-vue/blob/master/CONTRIBUTING.md)
to find information on project set-up, design information, and contributing
processes.

Visit the [OpenBMC Web UI Style Guide](https://openbmc.github.io/webui-vue/) to
find information on:

- Coding Standards
- Guidelines
- Unit Testing
- Components Usage
- Quick Start References

Visit the
[OpenBMC Web UI Themes Guide - How to customize](https://openbmc.github.io/webui-vue/themes/customize.html)
to learn how to create custom builds to meet your branding and customization
needs for:

- Routing
- Navigation
- State Store
- Theming

## Load Web UI against QEMU

Connect to Web UI in QEMU

1. You will need the QEMU session running per instructions in the "Download and
   Start QEMU Session" section of
   [dev-environment](https://github.com/openbmc/docs/blob/master/development/dev-environment.md).

2. Assuming you used the default of 2443 for the HTTPS port in your QEMU
   session, you will point your web browser to https://localhost:2443.

3. Login with default username and password and verify basic Web UI features are
   working as expected.

**Note** You will need to approve the security exception in your browser to
connect. OpenBMC is running with a self-signed SSL certificate.

# Phosphor-webui

The [phosphor-webui](https://github.com/openbmc/phosphor-webui) repository
provides a web-based interface for an OpenBMC. It uses the
[AngularJS](https://angularjs.org/) framework to interact with the BMC via REST
API calls. It allows users to view hardware information, update firmware, set
network settings, and much more.

See directions below to learn
[how to customize phosphor-webui](#customize-phosphor-webui).

Phosphor-webui was built on AngularJS and AngularJS goes End of Life June
30, 2021.

## Customize Phosphor-webui

1. Clone the repository

   ```
   git clone https://github.com/openbmc/phosphor-webui.git
   ```

2. Install appropriate packages and start web UI

   Follow the directions in the phosphor-webui
   [README](https://github.com/openbmc/phosphor-webui/blob/master/README.md) to
   install the required packages and start the web UI. You can use the
   development environment created in
   [dev-environment](https://github.com/openbmc/docs/blob/master/development/dev-environment.md)
   or your own system assuming you install the required packages noted in the
   README.

3. Customize the web UI login screen and verify

   Kill your npm run from the previous step using Ctrl^C. Grab a png that you
   will use to represent your customized version of OpenBMC. Feel free to use
   any .png you wish for this step.

   ```
   wget http://www.pngmart.com/files/3/Free-PNG-Transparent-Image.png
   ```

   Copy your new .png into the appropriate directory

   ```
   cp Free-PNG-Transparent-Image.png app/assets/images/
   ```

   Point to that new image in the web UI HTML

   ```
   vi app/login/controllers/login-controller.html
   # Replace the logo.svg near the top with Free-PNG-Transparent-Image.png
   <img src="../../assets/images/Free-PNG-Transparent-Image.png" class="login__logo" alt="OpenBMC logo" role="img"/>
   ```

   Start up the server with your change

   ```
   npm run-script server
   ```

   Load web browser at https://localhost:8080 and verify your new image is on
   the login screen.

   Kill your npm run using Ctrl^C.

4. Customize the header with the new image and verify The header is on every
   page in the web UI. It has a smaller version of the logo in it which we are
   changing with this step.

   Similar to the previous step, modify the appropriate HTML for the header:

   ```
   vi app/common/directives/app-header.html
   # Replace logo.svg with Free-PNG-Transparent-Image.png again
   <div class="logo__wrapper"><img src="../../assets/images/Free-PNG-Transparent-Image.png" class="header__logo" alt="company logo"/></div>
   ```

   Start up the server with your change

   ```
   npm run-script server
   ```

   Browse to https://localhost:8080 and verify your new image is on the header.

   Note that you will need to log in to view the header. Point the web UI to
   your QEMU session by typing the QEMU session (e.g. localhost:2443) in the
   "BMC HOST OR BMC IP ADDRESS" field.

   Note that in the HTML where you're replacing the images, there is also the
   corresponding text that goes with the images. Changing the text to match with
   your logo is also something you can easily do to customize your creation of
   an OpenBMC system.

   And that's it! You've downloaded, customized, and run the OpenBMC
   phosphor-webui code!
