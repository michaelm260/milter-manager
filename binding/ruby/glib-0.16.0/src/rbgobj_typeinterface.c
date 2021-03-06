/* -*- c-file-style: "ruby"; indent-tabs-mode: nil -*- */
/**********************************************************************

  rbgobj_typeinterface.c -

  $Author: mutoh $
  $Date: 2006/11/21 23:57:33 $
  created at: Sat May 27 16:04:13 JST 2006
 
  Copyright (C) 2002-2006  Ruby-GNOME2 Project Team
  Copyright (C) 2002,2003  Masahiro Sakai

**********************************************************************/

#include "global.h"

VALUE mMetaInterface;

static VALUE
interface_s_append_features(self, klass)
     VALUE self, klass;
{
    if (!rb_obj_is_kind_of(klass, cInstantiatable))
        rb_raise(rb_eTypeError, "Not a subclass of GLib::Instantiatable");
    return rb_call_super(1, &klass);
}

#if GLIB_CHECK_VERSION(2,4,0)
static VALUE
interface_install_property(self, pspec_obj)
    VALUE self, pspec_obj;
{
    const RGObjClassInfo* cinfo = rbgobj_lookup_class(self);
    gpointer ginterface;
    GParamSpec* pspec;

    if (cinfo->klass != self)
        rb_raise(rb_eTypeError, "%s isn't registered class",
                 rb_class2name(self));

    pspec = G_PARAM_SPEC(RVAL2GOBJ(pspec_obj));

    ginterface = g_type_default_interface_ref(cinfo->gtype);
    g_object_interface_install_property(ginterface, pspec);
    g_type_default_interface_unref(ginterface);

    /* FIXME: define accessor methods */
    return Qnil;
}

static VALUE
interface_property(self, property_name)
     VALUE self, property_name;
{
    gpointer ginterface;
    const char* name;
    GParamSpec* prop;
    VALUE result;

    if (SYMBOL_P(property_name)) {
        name = rb_id2name(SYM2ID(property_name));
    } else {
        StringValue(property_name);
        name = StringValuePtr(property_name);
    }

    if (CLASS2GTYPE(self) != RBGOBJ_TYPE_RUBY_VALUE){
        ginterface = g_type_default_interface_ref(CLASS2GTYPE(self));

        prop = g_object_interface_find_property(ginterface, name);
        if (!prop){
            g_type_default_interface_unref(ginterface);
            rb_raise(rb_const_get(mGLib, rb_intern("NoPropertyError")), 
                     "no such property: %s", name);
        }

        result = GOBJ2RVAL(prop);
        g_type_default_interface_unref(ginterface);
    } else {
        result = Qnil;
    }
    return result;
}

static VALUE
interface_properties(int argc, VALUE* argv, VALUE self)
{
    guint n_properties;
    GParamSpec** props;
    VALUE inherited_too;
    VALUE ary;
    int i;
    gpointer ginterface;

    GType gtype  = CLASS2GTYPE(self);
    ary = rb_ary_new();
    if (gtype != G_TYPE_INTERFACE){
        ginterface = g_type_default_interface_ref(gtype);

        if (rb_scan_args(argc, argv, "01", &inherited_too) == 0)
            inherited_too = Qtrue;
        
        props = g_object_interface_list_properties(ginterface, &n_properties);
        
        for (i = 0; i < n_properties; i++){
            if (RTEST(inherited_too)
                || GTYPE2CLASS(props[i]->owner_type) == self)
                rb_ary_push(ary, rb_str_new2(props[i]->name));
        }
        g_free(props);
        g_type_default_interface_unref(ginterface);
    }
    return ary;
}
#endif

void
rbgobj_init_interface(interf)
    VALUE interf;
{
    /* pseudo inheritance */
    if (CLASS2GTYPE(interf) != G_TYPE_INTERFACE){
        rb_extend_object(interf, GTYPE2CLASS(G_TYPE_INTERFACE));
        rb_include_module(interf, GTYPE2CLASS(G_TYPE_INTERFACE));
        rbgobj_define_property_accessors(interf);
    }
}

static void
Init_interface()
{
    VALUE iface = G_DEF_INTERFACE(G_TYPE_INTERFACE, "Interface", mGLib);

    rb_define_method(mMetaInterface, "append_features", interface_s_append_features, 1);
#if GLIB_CHECK_VERSION(2,4,0)
    rb_define_method(mMetaInterface, "install_property", interface_install_property, 1);
    rb_define_method(mMetaInterface, "property", interface_property, 1);
    rb_define_method(mMetaInterface, "properties", interface_properties, -1);
#endif

    rb_extend_object(iface, mMetaInterface);
    rb_include_module(iface, mMetaInterface);
}


void
Init_gobject_typeinterface()
{
    mMetaInterface = rb_define_module_under(mGLib, "MetaInterface");
    rb_define_method(mMetaInterface, "gtype", generic_gtype, 0);

    Init_interface();
}
