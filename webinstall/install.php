<?php
include("functions.php");
include("config.php");
const API_BASE="https://api.github.com/repos/#user#/#repo#";
$api = API_BASE."/releases/latest";
$branchsha=API_BASE."/git/refs/heads/#branch#";
$tagsha=API_BASE."/git/refs/tags/#tag#";
$download = "https://github.com/#user#/#repo#/releases/download/#dlVersion#/#dlName#";
$manifest = "?dlName=#mName#&dlVersion=#mVersion#&user=#user#&repo=#repo#";
$proxurl="https://raw.githubusercontent.com/#user#/#repo#/#sha#/#proxy#";
try {
	if (isset($_REQUEST['api'])) {
		$vars = fillUserAndRepo();
		proxy(replaceVars($api, $vars));
		exit(0);
	}
	if (isset($_REQUEST['branch'])){
		$vars = fillUserAndRepo();
		$vars = addVars($vars, array('branch'));
		proxy(replaceVars($branchsha, $vars));
		exit(0);
	}
	if (isset($_REQUEST['tag'])){
		$vars = fillUserAndRepo();
		$vars = addVars($vars, array('tag'));
		proxy(replaceVars($tagsha, $vars));
		exit(0);
	}
	if (isset($_REQUEST['dlName'])) {
		$vars = fillUserAndRepo();
		$vars = addVars($vars, array('dlName', 'dlVersion'));
		proxy(replaceVars($download, $vars));
		exit(0);
	}
	if (isset($_REQUEST['flash'])) {
		$vars = fillUserAndRepo();
		$json = getJson(replaceVars($api, $vars));
		$assets = $json['assets'];
		$targetUrl = null;
		$targetBase = $_REQUEST['flash'];
		$mode = 'all';
		if (isset($_REQUEST['update']))
			$mode = 'update';
		$lb = strlen($targetBase);
		foreach ($assets as &$asset) {
			if (substr($asset['name'], 0, $lb) == $targetBase) {
				if (!preg_match("/-$mode.bin/", $asset['name']))
					continue;
				$targetUrl = $asset['browser_download_url'];
				break;
			}
		}
		if (!$targetUrl)
			throw new Exception("unable to find $targetBase $mode\n");
		#echo("download for $targetBase=$targetUrl\n");
		proxy($targetUrl);
		exit(0);
	}
	if (isset($_REQUEST['proxy'])){
		$vars = fillUserAndRepo();
		$vars = addVars($vars, array('sha', 'proxy'));
		proxy(replaceVars($proxurl, $vars));
		exit(0);
	}
} catch (HTTPErrorException $h) {
	header($_SERVER['SERVER_PROTOCOL'] . " " . $h->code . " " . $h->getMessage());
	die($h->getMessage());
} catch (Exception $e) {
	header($_SERVER['SERVER_PROTOCOL'] . ' 500 ' . $e->getMessage());
	die($e->getMessage());
}
die("invalid request");
?>