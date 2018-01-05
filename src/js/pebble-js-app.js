var DEFAULT_TOTAL_HOURS = 8;
var DEFAULT_ACC_TOTAL_HOURS = 40;
var SEND_ELEMENTS_KEYMAP = 10;
var SEND_HOURS_KEYMAP = 1000;

function defaultTree() {
	return [
		{
		text: { name: "main" },
		children: [
			{
			text: { name: "work" },
			children: [
				{ text: { name: "hard", priority: 1 } },
				{ text: { name: "simple", priority: 1 } }
			]
			},
			{ text: { name: "education", priority: 2 } }
		]
		},
		{
		text: { name: "secondary" },
		children: [
			{
			text: { name: "additional" },
			children: [
				{ text: { name: "overview", priority: 3 } },
				{ text: { name: "optimization", priority: 3 } }
			]
			},
			{ text: { name: "distractions", priority: 4 } }
		]
		}
	];
}

function encodeTree(treeNodes, encTree, counter) {
	for(var i = 0; i < treeNodes.length; i++) {
		var node = treeNodes[i];
		var childs = node.children;
		if (childs) {
			counter.pairs_index++;
			var j = 2 * counter.pairs_index - 1;
			encTree[j] = childs[0].text.value + childs[1].text.value;
			encTree[j + 1] = node.text.value;
			encodeTree(childs, encTree, counter);
		}
		else {
			counter.elements_index++;
			var k = 2 * counter.elements_index - 1;
			encTree[SEND_ELEMENTS_KEYMAP * k] = node.text.value;
			encTree[SEND_ELEMENTS_KEYMAP * (k + 1)] = node.text.priority;
		}
	}
}

function appMessageAck() {
	console.log("tree sent to Pebble successfully");
}

function appMessageNack(e) {
	console.log("tree not sent to Pebble: " + JSON.stringify(e));
}

Pebble.addEventListener("showConfiguration", function() {
	var tree = JSON.parse(localStorage.getItem('tree'));
	var total = localStorage.getItem('total');
	var accTotal = localStorage.getItem('acctotal');
	var url='https://joker512.github.io/tracker.html?tree=';
	if (tree === null || total === null || accTotal === null) {
		tree = defaultTree();
		total = DEFAULT_TOTAL_HOURS;
		accTotal = DEFAULT_ACC_TOTAL_HOURS;
	}
	console.log("read tree = " + JSON.stringify(tree));
	url = url + encodeURIComponent(JSON.stringify(tree)) + "&total=" + total + "&acctotal=" + accTotal;
	console.log("url = " + url);
	Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed", function(e) {
	if (e.response !== '') {
		var data = JSON.parse(decodeURIComponent(e.response));
		console.log("got tree = " + JSON.stringify(data[0]));
		localStorage.setItem('tree', JSON.stringify(data[0]));
		localStorage.setItem('total', data[1]);
		localStorage.setItem('acctotal', data[2]);

		var encTree = {};
		var counter = {};
		counter.pairs_index = 0;
		counter.elements_index = 0;
		encodeTree(data[0], encTree, counter);
		encTree[SEND_HOURS_KEYMAP * 1] = parseInt(data[1]);
		encTree[SEND_HOURS_KEYMAP * 2] = parseInt(data[2]);
		console.log("encoded data = " + JSON.stringify(encTree));

		Pebble.sendAppMessage(encTree, appMessageAck, appMessageNack);
	}
	else {
		console.log("got no changes");
	}
});
