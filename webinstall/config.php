<?php
$allowed=array(
    'user'=> array('wellenvogel'),
    'repo'=> array('esp32-nmea2000')
);


function fillUserAndRepo($vars=null){
    global $allowed;
    if ($vars == null) {
        $vars=array();
    }
    foreach (array('user','repo') as $n){
        if (! isset($_REQUEST[$n])){
            die("missing parameter $n");
        }
        $v=$_REQUEST[$n];
        $av=$allowed[$n];
        if (! in_array($v,$av)){
            die("value $v for $n not allowed");
        }
        $vars[$n]=$v;
    }
    return $vars;
}
?>