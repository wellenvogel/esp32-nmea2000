<?php
function safeName($name)
{
    return preg_replace('[^0-9_a-zA-Z.-]', '', $name);
}
function replaceVars($str, $vars)
{
    foreach ($vars as $n => &$v) {
        $str = str_replace("#" . $n . "#", $v, $str);
    }
    return $str;
}
if (!function_exists('getallheaders')) {
    function getallheaders()
    {
        $headers = [];
        foreach ($_SERVER as $name => $value) {
            if (substr($name, 0, 5) == 'HTTP_') {
                $headers[str_replace(' ', '-', ucwords(strtolower(str_replace('_', ' ', substr($name, 5)))))] = $value;
            }
        }
        return $headers;
    }
}
function addVars(&$vars,$names,$defaults=null){
    foreach ($names as $n){
        $v=null;
        if (! isset($_REQUEST[$n])){
            if ($defaults == null || ! isset($defaults[$n])) die("missing parameter $n");
            $v=$defaults[$n];
        }
        else{
            $v=safeName($_REQUEST[$n]);
        }
        $vars[$n]=$v;
    }
    return $vars;
}

function curl_exec_follow(/*resource*/ $ch, /*int*/ &$maxredirect = null) {
    $mr = $maxredirect === null ? 5 : intval($maxredirect);
    if (ini_get('open_basedir') == '' && ini_get('safe_mode' == 'Off') && false) {
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, $mr > 0);
        curl_setopt($ch, CURLOPT_MAXREDIRS, $mr);
    } else {
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, false);
        if ($mr > 0) {
            $newurl = curl_getinfo($ch, CURLINFO_EFFECTIVE_URL);
            $rch = curl_copy_handle($ch);
            curl_setopt($rch, CURLOPT_HEADER, true);
            curl_setopt($rch, CURLOPT_NOBODY, true);
            curl_setopt($rch, CURLOPT_FORBID_REUSE, false);
            curl_setopt($rch, CURLOPT_RETURNTRANSFER, true);
            do {
                curl_setopt($rch, CURLOPT_URL, $newurl);
                $header = curl_exec($rch);
                if (curl_errno($rch)) {
                    $code = 0;
                } else {
                    $code = curl_getinfo($rch, CURLINFO_HTTP_CODE);
                    if ($code == 301 || $code == 302) {
                        preg_match('/Location:(.*?)\n/', $header, $matches);
                        $newurl = trim(array_pop($matches));
                    } else {
                        $code = 0;
                    }
                }
            } while ($code && --$mr);
            curl_close($rch);
            if (!$mr) {
                if ($maxredirect === null) {
                    trigger_error('Too many redirects. When following redirects, libcurl hit the maximum amount.', E_USER_WARNING);
                } else {
                    $maxredirect = 0;
                }
                return false;
            }
            curl_setopt($ch, CURLOPT_URL, $newurl);
        }
    }
    curl_setopt(
        $ch,
        CURLOPT_HEADERFUNCTION,
        function ($curl, $header) {
            header($header);
            return strlen($header);
        }
    );
    curl_setopt(
        $ch,
        CURLOPT_WRITEFUNCTION,
        function ($curl, $body) {
            echo $body;
            return strlen($body);
        }
    );
    header('Access-Control-Allow-Origin:*');
    return curl_exec($ch);
}

function setFw($curl,$aheaders=null){
    $headers=getallheaders();
    $FWHDR = ['User-Agent'];
    $outHeaders = array();
    foreach ($FWHDR as $k) {
        if (isset($headers[$k])) {
            array_push($outHeaders, "$k: $headers[$k]");
        }
    }
    if ($aheaders != null){
        foreach ($aheaders as $hk => $hv){
            array_push($outHeaders,"$hk: $hv");
        }
    }
    curl_setopt($curl, CURLOPT_HTTPHEADER, $outHeaders);
}
function getJson($url,$headers=null){
    $curl = curl_init();
    curl_setopt($curl, CURLOPT_URL,$url);
    curl_setopt($curl,CURLOPT_RETURNTRANSFER, true);
    setFw($curl,$headers);
    $response = curl_exec($curl);
    $httpcode = curl_getinfo($curl, CURLINFO_HTTP_CODE);
    #echo("curl exec for $url:$response:$httpcode\n");
    if($e = curl_error($curl)) {
        curl_close($curl);
        return array('error'=>$e);
    } else {
        if ($httpcode >= 300){
            curl_close($curl);
            return array('error'=>"HTTP code ".$httpcode);
        }
        curl_close($curl);
        return json_decode($response, true);
    }    
}
function proxy($url)
{
    $ch = curl_init($url);
    curl_setopt_array(
        $ch,
        [
            CURLOPT_RETURNTRANSFER => true,
            CURLOPT_CONNECTTIMEOUT => 30,
        ]
    );
    setFw($ch);
    $response = curl_exec_follow($ch);
    curl_close($ch);
}

?>