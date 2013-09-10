function toBraille(obj)
{
  let url="";
 
  if (obj.tagName == "A") {
    url=obj.href;
  }
 
  if (obj.tagName == "IMG") {
    url = obj.src;
  }

  if (url != undefined) {
    window.open("http://img2brl.delysid.org/?url="+url,'_blank');
  }
}

