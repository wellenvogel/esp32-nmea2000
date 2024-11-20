#!/bin/perl -w
use List::Util qw( min max );
$NoOfPages=2;
@Defaults=qw(Voltage WindRose OneValue TwoValues ThreeValues FourValues FourValues2 Clock RollPitch Battery2);
@Numbers=qw(one two three four five six seven eight nine ten );
%NoOfFields=qw( OneValue    1
                TwoValues   2
                ThreeValues 3
                FourValues  4
                FourValues2 4
                WindRoseFlex    6
                ApparentWind    0
                Autobahn    0
                Battery2    0
                Battery     0
                BME280      0
                Clock       0
                DST810      0
                Generator   0
                KeelPosition 0
                RollPitch   0
                RudderPosition  0
                Solar       0
                Voltage     0
                White       0
                WindRose    0
                );
#@Pages=qw(ApparentWind Autobahn Battery2 Battery BME280 Clock DST810 FourValues2 FourValues Generator KeelPosition OneValue RollPitch RudderPosition Solar ThreeValues TwoValues Voltage White WindRose WindRoseFlex);
@Pages=keys(%NoOfFields);
$MaxNoOFFIelds=max(values(%NoOfFields));


for ($PageNo=1;$PageNo<=$NoOfPages;$PageNo++){
    print "{\n"; 
    print "\t","\"name\": \"page", $PageNo,"type\",\n";
    print "\t","\"label\": \"Type\",\n";
    print "\t",'"type": "list",',"\n";
    print "\t",'"default": "';
    print "$Defaults[$PageNo-1]";
    print'"',"\n";
    print "\t",'"description": "Type of page for page ',$PageNo,'",',"\n";
    print "\t",'"list": [';
    for ($p=0;$p<=$#Pages;$p++) {
        print '"', $Pages[$p], '"' ;
        if ($p < $#Pages){print ","} 
        } 
    print "]\n";
    print "\t",'"category": "OBP60 Page 1",',"\n";
    print "\t",'"capabilities": {',"\n";
    print "\t\t",'"obp60":"true"',"\n";
    print "\t",'}',"\n";
    print '},',"\n";
    for ($FieldNo=1; $FieldNo<=$MaxNoOFFIelds;$FieldNo++){  
        print "{\n";
        print "\t",'"name": "page',$PageNo,'value',$FieldNo,'",',"\n";
        print "\t",'"label": "Field ',$FieldNo,'",',"\n";
        print "\t",'"type": "boatData",',"\n";
        print "\t",'"default": "",',"\n";
        print "\t",'"description": "The display for field ',$Numbers[$FieldNo-1],'",',"\n";
        print "\t",'"category": "OBP60 Page ',$PageNo,'",',"\n";
        print "\t",'"capabilities": {',"\n";
        print "\t",'    "obp60":"true"',"\n";
        print "\t",'},',"\n";
        print "\t",'"condition":[';
        foreach $page (@Pages) {
            if($NoOfFields{$page}>=$FieldNo){ 
                print '"page1type":"',$page,'"},';
            } 
            
        #{"page1type":"OneValue"},{"page1type":"TwoValues"},{"page1type":"ThreeValues"},{"page1type":"FourValues"},{"page1type":"FourValues2"},{"page1type":"WindRoseFlex"}
        } 
        print "],\n";
    print '},',"\n";
    } 
} 
   

