# Expired password cheatsheet

This shows how to handle OpenBMC images that have an expired password
which must be changed before the BMC can be accessed.
- The design is here: https://github.com/openbmc/docs/blob/master/designs/expired-password.md.
- You'll get the expired password behavior if you are building from
  the master branch after this commit: TODO-FUTURE commitid when
  available

How to change the BMC's initial expired password:
- The default credentials remain user=root and password=0penBmc.
  You'll need these to access the password change interface.
- If you access the BMC via its serial console (such as from QEMU),
  you can login with default credentials, and the system will ask you
  to change the password.
- If you create a network session to the BMC via its Redfish REST
  APIs, your initial session can be used to change the password.  See
  below for details.
- If you create a network session to the BMC via the /login URI, your
  initial session can be used to change the password.
    - Look for a message in the HTTP response message body like:
        - "extendedMessage": "The password for this account must be
          changed. PATCH the 'Password' property for the account under
          URI: /redfish/v1/AccountService/Accounts/<username>"
    - If that field is present, PATCH the new password like this:
      ```
      curl -X PATCH \
      -https://${bmc}/xyz/openbmc_project/user/root/action/SetPassword \
      -d '{"Password":"0penBmc123"}'
      ```
    - Then sign off and create a new session with the new password.
- If you create a network session to the BMC via SSH:
    - If you use the default Dropbear SSH server, you'll see a message
      that the password is expired and must be changed.  But you won't
      be able to change the password, and you won't be able to access
      the BMC.
    - If you've customized your BMC image to use the OpenSSH server,
      you can use its "expired password" dialog to change your
      password and then access the BMC.

- If you access the BMC via ipmitool, your username and expired
  password will be reported as unauthorized and you will not have
  access.  The ipmitool has no way to distinguish an expired password
  from and incorrect username and password.  You'll have to first
  change the password via OpenBMC user management.  Using the Redfish
  APIs is recommended for this.
- If you access the BMC via the openbmctool (ref:
  https://github.com/openbmc/openbmc-tools/blob/master/thalerj/openbmctool.py),
  (TODO: see https://github.com/openbmc/openbmc-tools/issues/53).
- If you access the BMC via xCAT (ref: https://xcat.org/), use the
  REST API support of your choice (Redfish or /login) as shown above.

The following Bash commands change an OpenBMC system's expired password via Redfish.
```
bmc=127.0.0.1
bmc_https_port=443
userid=root
password=0penBmc
newpassword=0penBmc123
temp_dir=.
curlparms=--insecure
function login-and-change-password-as-needed()
{
  : Create a session
  curl ${curlparms} -X POST -D ${temp_dir}/headers.txt \
     https://${bmc}:${bmc_https_port}/redfish/v1/SessionService/Sessions \
    -d '{"UserName":"'${userid}'", "Password":"'${password}'"}' |
    tee ${temp_dir}/results.txt
  : Check results  
  authtok=$(grep "^X-Auth-Token: " ${temp_dir}/headers.txt | cut -d' ' -f2 | tr -d '\r')
  if test -n "${authtok}"; then
    echo "Created session and got X-Auth-Token okay"
    sessid=$(grep -e '"Id":' ${temp_dir}/results.txt | cut -d\" -f4)
    echo "sessid='${sessid}'"
    : Detect if password change is needed and set PasswordChangeRequired URI as needed.
    read -r -d '' python_code_to_detect_PasswordChangeRequired << EOM
import sys,json
j = json.load(sys.stdin)
if "@Message.ExtendedInfo" in j:
  for err_info in j["@Message.ExtendedInfo"]:
      if "PasswordChangeRequired" in err_info["MessageId"]:
        sys.stdout.write(err_info["MessageArgs"][0])
EOM
    PasswordChangeRequiredURI=$(python -c "$python_code_to_detect_PasswordChangeRequired" <${temp_dir}/results.txt)
    if test -n "${PasswordChangeRequiredURI}"; then
      echo "The password is expired; trying to change the password"
    curl ${curlparms} -X PATCH -H "X-Auth-Token: ${authtok}"  \
       https://${bmc}:${bmc_https_port}/redfish/v1/SessionService/Sessions \
      -d '{"Password":"'${newpassword}'"}'
    fi
  else
    echo "Failed to create session or get X-Auth-Token" && false
  fi
}
login-and-change-password-as-needed
```

How it works: The `login-and-change-password-as-needed` function first
creates a Redfish session and collects its Session ID and X-Auth
tokens.  It then tests if the PasswordChangeRequired message is
present and sets the new password if required.  It is intended to be
illustrative only; for example, it does not perform error checking,
and it writes secrets to a file.  Please choose your own password.

Details:
- If you have changed the password and wish to set it back to 0penBmc,
  access the BMC shell running as the root user, and enter the
  following command:
  ```
  sed -i -e '/^root:/c\root:$1$UGMqyqdG$FZiylVFmRRfl9Z0Ue8G7e/:18148:0:99999:7:::' /etc/shadow
  ```
  Note the `passwd` command cannot be used because 0penBmc does not meet password complexity rules.
- To expire a user's password (such as root), first sign in the shell
  as root, then enter the following command: `passwd --expire root`
- If you've lost access to the BMC because you don't know any of its
  passwords, you may be able to factory reset the BMC using one of the
  following techniques.  Factory reset should change the BMC back to
  its default passwords.
    - If you have access to its serial interface, press any key to
      stop U-Boot and enter commands: `setenv rwreset true; saveenv;
      boot`
    - If you have access to its flash storage device, re-flash a new
      image onto the BMC.
    - If you have access to the BMC via its host interface, use that
      access to factory reset the BMC.
- Address questions to OpenBMC community forums: https://github.com/openbmc/openbmc#contact
