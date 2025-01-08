#!/bin/perl -w
#A tool to generate the part of config.json that deals with pages and fields.
#DEPRECATED, moved to get_set.py
die "Please use gen_set.py instead";
#List of all pages and the number of parameters they expect.
%NoOfFieldsPerPage=qw( 
                ApparentWind    0
                XTETrack    0
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
# No changes needed beyond this point
#max number of pages supported by OBP60
$NoOfPages=10;
#Default selection for each page
@Defaults=qw(Voltage WindRose OneValue TwoValues ThreeValues FourValues FourValues2 Clock RollPitch Battery2);
@Numbers=qw(one two three four five six seven eight nine ten);
@Pages=sort(keys(%NoOfFieldsPerPage));
$MaxNoOfFieldsPerPage=0; # inital value, gets updated with maximum entry from %NoOfFieldsPerPage


#find max. number of fields without additional modules
 foreach (values(%NoOfFieldsPerPage)){
    if ($_ > $MaxNoOfFieldsPerPage){
        $MaxNoOfFieldsPerPage=$_;
    }
 }

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
        print "\t",'}, ',"\n";
        print "\t",'"condition":[';
        foreach $page (@Pages) {
            if($NoOfFieldsPerPage{$page}>=$FieldNo){ 
                print '{"page',$PageNo,'type":"',$page,'"},';
            } 
        } 
        print "\b],\n";
    print '},',"\n";
    } 
    print "{\n";
    print "\t","\"name\": \"page", $PageNo,"fluid\",\n";
    print "\t",'"label": "Fluid type",',"\n";
    print "\t",'"type": "list",',"\n";
    print "\t",'"default": "0",',"\n";
    print "\t",'"list": [',"\n";
    print "\t",'{"l":"Fuel (0)","v":"0"},',"\n";
    print "\t",'{"l":"Water (1)","v":"1"},',"\n";
    print "\t",'{"l":"Gray Water (2)","v":"2"},',"\n";
    print "\t",'{"l":"Live Well (3)","v":"3"},',"\n";
    print "\t",'{"l":"Oil (4)","v":"4"},',"\n";
    print "\t",'{"l":"Black Water (5)","v":"5"},',"\n";
    print "\t",'{"l":"Fuel Gasoline (6)","v":"6"}',"\n";
    print "\t",'],',"\n";
    print "\t",'"description": "Fluid type in tank",',"\n";
    print "\t",'"category": "OBP60 Page ',$PageNo,'",',"\n";
    print "\t",'"capabilities": {',"\n";
    print "\t",'"obp60":"true"',"\n";
    print "\t",'},',"\n";
    print "\t",'"condition":[{"page',$PageNo,'type":"Fluid"}]',"\n";
    print '},',"\n";
} 
