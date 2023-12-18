<?php
	$api="https://api.github.com/repos/#user#/#repo#/releases/latest";
	$download="https://github.com/#user#/#repo#/releases/download/#dlVersion#/#dlName#";
	$manifest="?dlName=#mName#&dlVersion=#mVersion#&user=#user#&repo=#repo#";
	$allowed=array(
		'user'=> array('wellenvogel'),
		'repo'=> array('esp32-nmea2000')
	);
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
	function safeName($name){
		return preg_replace('[^0-9_a-zA-Z.-]','',$name);
	}
	function replaceVars($str,$vars){
		foreach ($vars as $n => &$v){
			$str=str_replace("#".$n."#",$v,$str);
		}
		return $str;
	}

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
	function addVars($vars,$names){
		foreach ($names as $n){
			if (! isset($_REQUEST[$n])){
				die("missing parameter $n");
			}
			$safe=safeName($_REQUEST[$n]);
			$vars[$n]=$safe;
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
	function proxy($url)
	{
		$headers=getallheaders();
		$ch = curl_init($url);
		curl_setopt_array(
			$ch,
			[
				CURLOPT_RETURNTRANSFER => true,
				CURLOPT_CONNECTTIMEOUT => 30,
			]
		);
		$FWHDR = ['User-Agent'];
		$outHeaders = array();
		foreach ($FWHDR as $k) {
			if (isset($headers[$k])) {
				array_push($outHeaders, "$k: $headers[$k]");
			}
		}
		curl_setopt($ch, CURLOPT_HTTPHEADER, $outHeaders);
		$response = curl_exec_follow($ch);
		curl_close($ch);
	}

	if (isset($_REQUEST['api'])) {
		$vars=fillUserAndRepo();
		proxy(replaceVars($api,$vars));
		exit(0);
	}
	if (isset($_REQUEST['dlName'])){
		$vars=fillUserAndRepo();
		$vars=addVars($vars,array('dlName','dlVersion'));
		proxy(replaceVars($download,$vars));
		exit(0);
	}
	die("invalid request");
	?>