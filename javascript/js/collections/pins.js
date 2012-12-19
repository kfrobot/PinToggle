var app = app || {};

(function() {
	'use strict';

	var PinList = Backbone.Collection.extend({
		// Reference to this collection's model.
		model: app.Pin,
        url:"pins"
	});

	// Create our global collection of **Pins**.
	app.Pins = new PinList();
}());
