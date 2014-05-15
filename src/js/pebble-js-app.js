Pebble.addEventListener("ready",
  function(e) {
    console.log("JavaScript app ready and running!");
    getLocation();
  }
);

Pebble.addEventListener('getWeather', function(e) {
  getLocation();
});

function getLocation()
{
  if (navigator.geolocation) {
      navigator.geolocation.getCurrentPosition(getWeather);
  } else {
    console.log("Can't get location.");
  }
}

function getWeather(position)
{
  var url = "http://api.openweathermap.org/data/2.5/weather?lat=" 
  	+ position.coords.latitude + "&lon=" + position.coords.longitude;
  console.log(url);
  var req = new XMLHttpRequest();
    req.open('GET', url, true);
    req.onload = function(e) {
      if (req.readyState == 4 && req.status == 200) {
        if(req.status == 200) {
          var response = JSON.parse(req.responseText);
          var kelvin = response.main.temp;
          var temperature = Math.floor(1.8 * (kelvin - 273) + 32);
          console.log("Temperature: " + temperature);
          Pebble.sendAppMessage({ "temperature":temperature });
          console.log("Sent");
      } else { console.log("Error"); }
    }
  }
  req.send(null);
}