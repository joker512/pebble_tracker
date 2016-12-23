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
	]
}

function encodeTree(treeNodes, encTree, counter) {
	for(var i = 0; i < treeNodes.length; i++) {
		var node = treeNodes[i];
		var childs = node.children;
		if (childs) {
			counter.inc++;
			var j = counter.inc;
			encTree[2 * j - 1] = childs[0].text.value + childs[1].text.value;
			encTree[2 * j] = node.text.value;
			encodeTree(childs, encTree, counter);
		}
		else {
			counter.dec--;
			var k = counter.dec;
			encTree[2 * k + 1] = node.text.value;
			encTree[2 * k] = node.text.priority;
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
	var url='https://joker512.github.io/tracker.html?tree=';
	if (tree == null) {
		tree = defaultTree();
	}
	console.log("read tree = " + JSON.stringify(tree));
	url = url + encodeURIComponent(JSON.stringify(tree));
	console.log("url = " + url);
	Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed", function(e) {
	if (e.response !== '') {
		var tree = JSON.parse(decodeURIComponent(e.response));
		console.log("got tree = " + JSON.stringify(tree));
		localStorage.setItem('tree', JSON.stringify(tree));

		var encTree = {};
		var counter = {};
		counter.inc = 0;
		counter.dec = 0;
		encodeTree(tree, encTree, counter);
		console.log("encoded tree = " + JSON.stringify(encTree));

		Pebble.sendAppMessage(encTree, appMessageAck, appMessageNack);
	}
	else {
		console.log("got no changes");
	}
});
