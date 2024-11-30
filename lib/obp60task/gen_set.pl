#!/bin/perl -w
#A tool to generate that part of config.json  that deals with pages and Fields.

use List::Util qw( min max );


#List of all Pages and the number of parameters they expect.
%NoOfFieldsPerPage=qw( 
                ApparentWind    0
                Autobahn    0
                Battery2    0
                Battery     0
                BME280      0
                Clock       0
                DST810      0
                FourValues2  4
                FourValues 4
                Generator   0
                KeelPosition 0
                OneValue    1
                RollPitch   0
                RudderPosition  0
                Solar       0
                ThreeValues 3
                TwoValues   2
                Voltage     0
                White       0
                WindRose    0
                WindRoseFlex    6
                );
# No changes neede beyond this pint
$NoOfPages=10;
@Defaults=qw(Voltage WindRose OneValue TwoValues ThreeValues FourValues FourValues2 Clock RollPitch Battery2);
@Numbers=qw(one two three four five six seven eight nine ten);
@Pages=sort(keys(%NoOfFieldsPerPage));
$MaxNoOfFieldsPerPage=max(values(%NoOfFieldsPerPage));


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
    print "\t",'"category": "OBP60 Page ',$PageNo,'",',"\n";
    print "\t",'"capabilities": {',"\n";
    print "\t\t",'"obp60":"true"',"\n";
    print "\t",'}',"\n";
    print "\t",'"condition":[';
    for ($vp=$PageNo;$vp<=$NoOfPages;$vp++){ 
        print '"{visiblePages":"',$vp,'"},';
    } 
    #"visiblePages":"2"},{"visiblePages":"3"},{"visiblePages":"4"},{"visiblePages":"5"},{"visiblePages":"6"},{"visiblePages":"7"},{"visiblePages":"8"},{"visiblePages":"9"},{"visiblePages":"10"}
    print "\b",']',"\n";
    print '},',"\n";
    for ($FieldNo=1; $FieldNo<=$MaxNoOfFieldsPerPage;$FieldNo++){  
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
            if($NoOfFieldsPerPage{$page}>=$FieldNo){ 
                print '{"page1type":"',$page,'"},';
            } 
            
        #{"page1type":"OneValue"},{"page1type":"TwoValues"},{"page1type":"ThreeValues"},{"page1type":"FourValues"},{"page1type":"FourValues2"},{"page1type":"WindRoseFlex"}
        } 
        print "\b],\n";
    print '},',"\n";
    } 
} 
   

