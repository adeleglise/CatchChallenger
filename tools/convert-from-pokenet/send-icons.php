<?php

$dir = "./";
$dest='/home/user/Desktop/CatchChallenger/Grab-test/monsters/';

if ($dh = opendir($dir)) {
    while (($file = readdir($dh)) !== false) {
	if(preg_match('#[0-9]{3}\.gif#',$file))
	{
		$number=(int)str_replace('.gif','',$file);
		if(!is_dir($dest.$number.'/'))
			mkdir($dest.$number.'/');
		if(!file_exists($dest.$number.'/small.gif') && !file_exists($dest.$number.'/small.png'))
			copy($file,$dest.$number.'/small.gif');
//        	echo "filename: $file : filetype: " . filetype($dir . $file) . "\n";
	}
	if(preg_match('#[0-9]{3}\.png#',$file))
	{
		$number=(int)str_replace('.png','',$file);
		if(!is_dir($dest.$number.'/'))
			mkdir($dest.$number.'/');
		if(!file_exists($dest.$number.'/small.gif') && !file_exists($dest.$number.'/small.png'))
			copy($file,$dest.$number.'/small.png');
//        	echo "filename: $file : filetype: " . filetype($dir . $file) . "\n";
	}
    }
    closedir($dh);
}

