
const char indexHTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=UTF-8">
<title>NMEA 2000 Gateway</title>

<script type="text/javascript">
  function reset(){
    fetch('/api/reset');
  }
</script>  
<style type="text/css">
#wrap
{
text-align: center; 
vertical-align: middle; 
max-width: 900px; 
margin:0 auto;
background:#dcd;
border-radius: 10px;
padding:10px;       /*innen*/
}


</style>
</head>
<body>
<div class="main">
<h1>NMEA 2000 Gateway </h1>
<button id="reset" onclick="reset();">Reset</button>
</div>
</body>
</html>
)=====" ;


