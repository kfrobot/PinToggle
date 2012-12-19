var app = app || {};
$(function() {
	'use strict';

	// Todo Item View
	// --------------

	app.SwitchView = Backbone.View.extend({

		tagName:  'div',
        className : 'switch',

		// Cache the template function for a single item.
		template: _.template('<input class=\"toggle\" type=\"checkbox\" <%= low ? \'checked\' : \'\' %>> <label><b><%- pin %><\/b><\/label>'),

		// The DOM events specific to an item.
		events: {
			'click .toggle':	'toggleLow'
		},

		// Re-render the titles of the todo item.
		render: function() {
			this.$el.html( this.template( this.model.toJSON() ) );
			return this;
		},

		// Toggle the `"low"` state of the model.
		toggleLow: function() {
			this.model.toggle();
		}
	});
});
