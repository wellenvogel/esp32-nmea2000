<?php
	include("functions.php");
	include("config.php");
	$api="https://api.github.com/repos/#user#/#repo#/releases/latest";
	$download="https://github.com/#user#/#repo#/releases/download/#dlVersion#/#dlName#";
	$manifest="?dlName=#mName#&dlVersion=#mVersion#&user=#user#&repo=#repo#";
	
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
	if (isset($_REQUEST['flash'])){
        $vars=fillUserAndRepo();
		$json=getJson(replaceVars($api,$vars));
		$assets=$json['assets'];
		$targetUrl=null;
		$targetBase=$_REQUEST['flash'];
		$mode='all';
		if (isset($_REQUEST['update'])) $mode='update';
		$lb=strlen($targetBase);
		foreach ($assets as &$asset){
            if (substr($asset['name'],0,$lb) == $targetBase){
                if (! preg_match("/-$mode.bin/",$asset['name'])) continue;
                $targetUrl=$asset['browser_download_url'];
                break;
            }
		}
		if (! $targetUrl) die("unable to find $targetBase $mode\n");
		#echo("download for $targetBase=$targetUrl\n");
		proxy($targetUrl);
		exit(0);
	}
	die("invalid request");
	?>
