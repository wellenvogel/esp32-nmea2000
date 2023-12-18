<?php
$allowed=array(
    'user'=> array('wellenvogel'),
    'repo'=> array('esp32-nmea2000')
);


function fillUserAndRepo($vars=null,$source=null){
    global $allowed;
    if ($vars == null) {
        $vars=array();
    }
    if ($source == null){
        $source=$_REQUEST;
    }
    foreach (array('user','repo') as $n){
        if (! isset($source[$n])){
            throw new Exception("missing parameter $n");
        }
        $v=$source[$n];
        $av=$allowed[$n];
        if (! in_array($v,$av)){
            throw new Exception("value $v for $n not allowed");
        }
        $vars[$n]=$v;
    }
    return $vars;
}
?>