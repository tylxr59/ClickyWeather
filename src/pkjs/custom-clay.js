module.exports = function(minified) {
  var clayConfig = this;
  var $ = minified.$;

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var request = clayConfig.getItemByMessageKey('CheckForAppUpdate');
    var button = clayConfig.getItemById('check-app-update-now');

    request.hide();
    button.on('click', function() {
      request.set(true);
      $('#main-form').trigger('submit');
    });
  });
};
