var app = app || {};

(function() {
	app.Pin = Backbone.Model.extend({
        urlRoot:"pins",

		// Toggle the `low` state of this todo item.
		toggle: function() {
            console.log("saving");
			this.save({
				low: !this.get('low')
			});
		}
	});
}());
