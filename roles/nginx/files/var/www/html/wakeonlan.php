<?php

/*
 * /etc/sudoers.d/wakeonlan:
 * www-data ALL=NOPASSWD: /usr/bin/wakeonlan
 */

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
  $mac_address = $_POST['mac_address'];
  shell_exec("sudo /usr/bin/wakeonlan -i 172.19.10.255 $mac_address 2>&1");
  echo "Wakeup command sent to $mac_address";
} else {
  http_response_code(404);
  echo "Not found";
}
