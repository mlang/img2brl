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
    window.open(document.getElementById("img2brl-urls").getFormattedString("url", [url]), '_blank');
  }
}

