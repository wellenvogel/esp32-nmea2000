<?php
echo "CREATEDB\n";
include("functions.php");
$database=openDb();
if (! isset($database)){
    die("ERROR: db not set\n");
}
try{
$sql="CREATE TABLE `CIBUILDS` (
  `id` varchar(255) NOT NULL,
  `tag` varchar(255),
  `status` varchar(255) NOT NULL,
  `config` longtext,
  `environment` VARCHAR(255),
  `buildflags` longtext,
  `timestamp` timestamp ,
  PRIMARY KEY  (`id`)
);";

echo "executing $sql<br>";
$rt=$database->query($sql);
echo "execute OK<br>";
} catch (Exception $e){
    echo "ERROR: ".$e;
}


?>