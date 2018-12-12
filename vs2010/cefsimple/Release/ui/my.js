
var DB = {

_db : null,	
	
initDBDriver : function() {
	if (! this._db) {
		this._db = openDBDriver('mysql://localhost:3306/tdx_f10', 'root', 'root');
	}
	return this._db;
},

metaToArray : function(meta) {
	var title = [];
	var colNum = meta.getColumnCount();
	for (var i = 0; i < colNum; ++i) {
		title.push(meta.getColumnLabel(i));
	}
	return title;
},

/**
*	@param rs ResultSet object
*   @param columnNames An array of string, result set column names
*   @return An array of [{}, {}, ...]
*/
resultToArray : function (rs, columnNames) {
	var meta = rs.getMetaData();
	var colNum = meta.getColumnCount();
	var data = [];
	var title = [];
	if (columnNames && Array.isArray(columnNames) && colNum <= columnNames.length) {
		title = columnNames;
	} else {
		title = this.metaToArray(meta);
	}
	
	while (rs.next()) {
		var robj = {};
		for (var i = 0; i < colNum; ++i) {
			robj[title[i]] = rs.getString(i);
			// alert(title[i] + "  -> " + robj[title[i]]);
		}
		data.push(robj);
	}
	return data;
},

query : function(sql) {
	var db = this.initDBDriver();
	var rs = db.executeQuery(sql);
	var data = this.resultToArray(rs);
	rs.close();
	return data;
},

/**
*	@bindParams An arroy of string, sql bind params
*	@return return An array of record object
*/
prepareQuery : function (sql, bindParams) {
	var db = this.initDBDriver();
	var ps = db.prepare(sql);
	var paramNum = ps.getParameterCount();
	if (paramNum > 0 && paramNum != bindParams.length) {
		alert('prepareQuery: bind params number error');
		return null;
	}
	for (var i = 0; i < paramNum; ++i) {
		ps.setString(i, bindParams[i]);
	}
	var rs = ps.executeQuery();
	var data = this.resultToArray(rs);
	
	rs.close();
	ps.close();
	
	return data;
}

};

var ComboBox = {
	/**
	*	load combobox data from sql
	*   @param sql select val, txt from xx_table. Must has only 2 column
	*	@return [{value:'', text:''}, ...]
	*/
	queryData : function(sql, bindParams) {
		var db = DB.initDBDriver();
		var ps = db.prepare(sql);
		var paramNum = ps.getParameterCount();
		if (paramNum > 0 && paramNum != bindParams.length) {
			alert('prepareQuery: bind params number error');
			return null;
		}
		for (var i = 0; i < paramNum; ++i) {
			ps.setString(i, bindParams[i]);
		}
		var rs = ps.executeQuery();
		var data = [];
		while (rs.next()) {
			var v = rs.getString(0);
			var t = rs.getString(1);
			data.push({value:v, text:t});
		}
		rs.close();
		ps.close();
		return data;
	}
};

var Tab = {
	/**
	* @param tabObj An tab's jquery obj.  Eg: $('#xx_tab')
	* @param title  An tab item title
	* @param url An tab item url
	* @param tabId Unique tab item id. If not given, same as url
	*     Eg: openTab(, 'My Title', 'abc.html')
	*/
	openTab: function (tabObj, title, url, tabId) {
		var opts = tabObj.tabs('tabs');
		var len = opts.length;
		if (!tabId) {
			tabId = url;
		}
		for (var i = 0; i < len; ++i) {
			var item = tabObj.tabs('getTab', i);
			if (item._tabId == tabId) {
				tabObj.tabs('select', i);
				return;
			}
		}
		tabObj.tabs('add',{
			title: title,
			closable: true,
			fit:true, border:true,
			content:'<iframe frameborder="0" width="100%" height="100%" scrolling="auto" src="' + url + '" />'
		});
		var item = tabObj.tabs('getTab', title);
		item._tabId = tabId;
	}
};

var Request = {
	_params : {},
	_hasInit : false,
	_init : function() {
		if (this._hasInit) return;
		this._hasInit = true;
		var url = window.location.href;
		var idx = url.indexOf('?');
		if (idx < 0) return;
		var str = url.substring(idx + 1);
		if (! str) return;
		var list = str.split('&');
		for (var i = 0; i < list.length; ++i) {
			if (! list[i]) continue;
			var kv = list[i].split('=');
			if (kv.length == 2 && kv[0]) {
				this._params[kv[0]] = kv[1];
			}
		}
	},
	
	getParam : function (paramName) {
		this._init();
		return this._params[paramName];
	},
	
	getParams : function() {
		this._init();
		return this._params;
	}
};







