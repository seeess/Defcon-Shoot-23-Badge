#!/usr/bin/perl
#@author twitter@see_ess
#this script converts ascii text from $inputstring to 
#integers 0-35 (which equate to a-z and 0-9) along with the 
#equals character delimiter being converted to "100". These 
#integer values are used by the badge firmware to store built 
#in morse code strings and converted into morse code blink 
#patterns see morsestr[] and morsecodelookup[] for more context

$inputstring = 
"defcon23=SOS=hack everything=break shit=nothing is impossible=fuck the NSA=i dont play well with others=what are you doing dave=youre either a one or a zero Alive or dead=danger zone=bros before apparent threats to national security=im spooning a Barrett 50 cal i could kill a building=there is no spoon=never send a human to do a machines job=guns lots of guns=its not that im lazy its just that i dont care=PC load letter=shall we play a game=im getting too old for this=censorship reveals fear=the right of the people to keep and bear Arms shall not be infringed=all men having power ought to be mistrusted=when governments fear the people there is liberty=";
$outputstring = "";	

for($i = 0;$i < length($inputstring); $i++){
	$dig = ord(substr($inputstring, $i,$i+1));
	#cap A-Z
	if($dig > 64 && $dig < 91){
		$dig = $dig + 32;
	}
	#digits 0-9
	if($dig > 47 && $dig < 58){
		$dig = $dig - 22;
	#delim
	}elsif($dig == 61){
		$dig = 100;
	#lowercase a-z
	}else{
		$dig = $dig - 97;
	}
	print $dig . ',';
}
