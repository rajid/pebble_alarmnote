var initialised = false;
var options;


Pebble.addEventListener("ready",
			function(e) 
			{
			    console.log("JavaScript app here!");
                            Pebble.sendAppMessage({'AppKeyReady': true});
			    initialised = true;
			}
    );


Pebble.addEventListener("showConfiguration", function() 
			{
			    var options = JSON.parse(localStorage.getItem('options'));
			    console.log("read options: " + JSON.stringify(options));
			    console.log("showing configuration");
			    var url='http://mischievous.us/config4.html';
			    
                            Pebble.getTimelineToken();
                            
			    if (options !== null) {
                                url = url + '?';
                                if (typeof options.a1 == 'undefined') {
                                    url = url + 'a1=';
                                } else {
                                    url = url + 'a1=' + encodeURIComponent(options.a1);
                                }
                                url = url + '&';
                                if (typeof options.mess1 == 'undefined') {
                                    url = url + 'mess1=';
                                } else {
                                    url = url + 'mess1=' + encodeURIComponent(options.mess1);
                                }
                                url = url + '&';
                                if (typeof options.a2 == 'undefined') {
                                    url = url + 'a2=';
                                } else {
                                    url = url + 'a2=' + encodeURIComponent(options.a2);
                                }
                                url = url + '&';
                                if (typeof options.mess2 == 'undefined') {
                                    url = url + 'mess2=';
                                } else {
                                    url = url + 'mess2=' + encodeURIComponent(options.mess2);
                                }
                                url = url + '&';
                                if (typeof options.a3 == 'undefined') {
                                    url = url + 'a3=';
                                } else {
                                    url = url + 'a3=' + encodeURIComponent(options.a3);
                                }
                                url = url + '&';
                                if (typeof options.mess3 == 'undefined') {
                                    url = url + 'mess3=';
                                } else {
                                    url = url + 'mess3=' + encodeURIComponent(options.mess3);
                                }
                                url = url + '&';
                                if (typeof options.a4 == 'undefined') {
                                    url = url + 'a4=';
                                } else {
                                    url = url + 'a4=' + encodeURIComponent(options.a4);
                                }
                                url = url + '&';
                                if (typeof options.mess4 == 'undefined') {
                                    url = url + 'mess4=';
                                } else {
                                    url = url + 'mess4=' + encodeURIComponent(options.mess4);
                                }
                                url = url + '&';
                                if (typeof options.a5 == 'undefined') {
                                    url = url + 'a5=';
                                } else {
                                    url = url + 'a5=' + encodeURIComponent(options.a5);
                                }
                                url = url + '&';
                                if (typeof options.mess5 == 'undefined') {
                                    url = url + 'mess5=';
                                } else {
                                    url = url + 'mess5=' + encodeURIComponent(options.mess5);
                                }
                                url = url + '&';
                                if (typeof options.a6 == 'undefined') {
                                    url = url + 'a6=';
                                } else {
                                    url = url + 'a6=' + encodeURIComponent(options.a6);
                                }
                                url = url + '&';
                                if (typeof options.mess6 == 'undefined') {
                                    url = url + 'mess6=';
                                } else {
                                    url = url + 'mess6=' + encodeURIComponent(options.mess6);
                                }
                                url = url + '&';
                                if (typeof options.a7 == 'undefined') {
                                    url = url + 'a7=';
                                } else {
                                    url = url + 'a7=' + encodeURIComponent(options.a7);
                                }
                                url = url + '&';
                                if (typeof options.mess7 == 'undefined') {
                                    url = url + 'mess7=';
                                } else {
                                    url = url + 'mess7=' + encodeURIComponent(options.mess7);
                                }
                                url = url + '&';
                                if (typeof options.snooze == 'undefined') {
                                    url = url + 'snooze=';
                                } else {
                                    url = url + 'snooze=' + encodeURIComponent(options.snooze);
                                }
                                url = url + '&';
                                if (typeof options.snoozemsg == 'undefined') {
                                    url = url + 'snoozemsg=';
                                } else {
                                    url = url + 'snoozemsg=' + encodeURIComponent(options.snoozemsg);
                                }
                                url = url + '&';
                                if (typeof options.fontsize == 'undefined') {
                                    url = url + 'fontsize=';
                                } else {
                                    url = url + 'fontsize=' + encodeURIComponent(options.fontsize);
                                }
			    }
			    console.log("url=" + url);
			    Pebble.openURL(url);
			}
    );


function appMessageAck(e) 
{
    console.log("options sent to Pebble successfully");
}


function appMessageNack(e) 
{
    if (e.error) 
	console.log("options not sent to Pebble: " + e.error.message);
}
    

Pebble.addEventListener("webviewclosed", function(e) 
			{
			    console.log("configuration closed");
			    if (e.response !== '') {
				options = JSON.parse(decodeURIComponent(e.response));
				console.log("Options = " + JSON.stringify(options));
				localStorage.setItem('options', JSON.stringify(options));
				Pebble.sendAppMessage(options, appMessageAck, appMessageNack);
			    }
			}
    );

Pebble.addEventListener("appmessage", function(e)
			{
                            console.log("Received message from watch" + JSON.stringify(e.payload));
                            if (e.payload.a1 && e.payload.mess1) {
                                console.log("a1=" + e.payload.a1);
                                pushPin(e.payload.a1, e.payload.mess1, "0001");
                            } else {
                                pushPin("", "", "0001");
                            }
                            if (e.payload.a2 && e.payload.mess2) {
                                console.log("a2=" + e.payload.a2);
                                pushPin(e.payload.a2, e.payload.mess2, "0002");
                            } else {
                                pushPin("", "", "0002");
                            }
                            if (e.payload.a3 && e.payload.mess3) {
                                console.log("a3=" + e.payload.a3);
                                pushPin(e.payload.a3, e.payload.mess3, "0003");
                            } else {
                                pushPin("", "", "0003");
                            }
                            if (e.payload.a4 && e.payload.mess4) {
                                console.log("a4=" + e.payload.a4);
                                pushPin(e.payload.a4, e.payload.mess4, "0004");
                            } else {
                                pushPin("", "", "0004");
                            }
                            if (e.payload.a5 && e.payload.mess5) {
                                console.log("a5=" + e.payload.a5);
                                pushPin(e.payload.a5, e.payload.mess5, "0005");
                            } else {
                                pushPin("", "", "0005");
                            }
                            if (e.payload.a6 && e.payload.mess6) {
                                console.log("a6=" + e.payload.a6);
                                pushPin(e.payload.a6, e.payload.mess6, "0006");
                            } else {
                                pushPin("", "", "0006");
                            }
                            if (e.payload.a7 && e.payload.mess7) {
                                console.log("a7=" + e.payload.a7);
                                pushPin(e.payload.a7, e.payload.mess7, "0007");
                            } else {
                                pushPin("", "", "0007");
                            }
                        }
    );


// The timeline public URL root
var API_URL_ROOT = 'https://timeline-api.getpebble.com/';

function timelineRequest(pin, type, callback) {

    Pebble.getTimelineToken(
        function (token) 
        {
            // User token pin (not shared)
            var url = API_URL_ROOT + 'v1/user/pins/' + pin.id;

            // Create XHR
            var xhr = new XMLHttpRequest();
            xhr.onload = function () {
                console.log('timeline: response received: ' + this.responseText);
                callback(this.responseText);
            };

            // Add headers
            xhr.setRequestHeader("Content-Type", "application/json");
            console.log('Using user token:' + token);
            xhr.setRequestHeader("X-User-Token", token);

            xhr.open(type, url);

            // Send
            if (type == 'PUT') {
                xhr.send(JSON.stringify(pin));
            } else {
                xhr.send('');
            }
            console.log('timeline: request sent.');
        },
        function(error) { console.log('timeline: error getting my timeline token: ' + error); });
}

function insertUserPin(pin, callback) {
  timelineRequest(pin, 'PUT', callback);
}

function deleteUserPin(pin, callback) {
  timelineRequest(pin, 'DELETE', callback);
}

// push a pin for "h"=hour, "m"=minutes, "t"=text
function pushPin(t, mess, idx)
{
    
    console.log("t = " + t);
    var now = new Date(t*1000);              // date for change
    console.log("pushpin date = " + now);

    // Create the pin
    var pin = {
        "id": "pin-" + idx,
        "time": now.toISOString(),
        "layout": {
            "type": "genericPin",
            "title": mess,
            "tinyIcon": "system://images/SCHEDULED_EVENT"
        }
    };

    if (mess != '') {
        console.log('Inserting pin in the future: ' + JSON.stringify(pin));
        insertUserPin(pin, function(responseText) { 
                console.log('Result: ' + responseText);
            });
    } else {
        console.log('Deleting pin in the future at ' + now.toISOString());
        deleteUserPin(pin, function(responseText) { 
                console.log('Result: ' + responseText);
            });
    }
}


