/**
 * Heli Loader.
 * 
 * Loads the required JS and CSS for the Helirig viewer
 * 
 * Author:
 * Jos Craw
 */

function loadHeliJS(filePath) {
    $('head').append('<script type="text/javascript" src="'+filePath+'"></script>');
}

function loadHeliCSS(filePath) {
    $('head').append('<link href="'+filePath+'" rel="stylesheet" type="text/css">');
}

function loadHeliFiles() {
    loadHeliJS('https://storage.googleapis.com/heli.ucquarantine.net/v0.1.2/heli_key_bindings.js');
    loadHeliCSS('https://cdn.metroui.org.ua/v4.3.2/css/metro-all.min.css');
    loadHeliJS('https://cdn.metroui.org.ua/v4.3.2/js/metro.min.js');
    loadHeliJS('https://storage.googleapis.com/heli.ucquarantine.net/v0.1.2/heli_menu.js');

    loadHeliJS('https://storage.googleapis.com/heli.ucquarantine.net/v0.1.2/redesign_heli.js')
}

loadHeliFiles();
