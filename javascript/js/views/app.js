var app = app || {};

$(function( $ ) {
	'use strict';
	app.AppView = Backbone.View.extend({
		initialize: function() {
			window.app.Pins.on( 'reset', this.addAll, this );
			app.Pins.fetch();
		},
		addOne: function( todo ) {
			var view = new app.SwitchView({ model: todo });
			$('#pin-list').append( view.render().el );
		},
		addAll: function() {
			this.$('#pin-list').html('');
			app.Pins.each(this.addOne, this);
		}
	});
});
