package com.ariasaproject.poolminerlite.views;

import android.content.Context;
import android.content.res.TypedArray;
import android.support.annotation.IdRes;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.RadioButton;

public class CustomTabLayout extends LinearLayout {
    // holds the checked id; the selection is empty by default
    private int mCheckedId = -1;
    // tracks children radio buttons checked state
    private CompoundButton.OnCheckedChangeListener mChildOnCheckedChangeListener;
    // when true, mOnCheckedChangeListener discards events
    private boolean mProtectFromCheckedChange = false;
    private OnCheckedChangeListener mOnCheckedChangeListener;
    private PassThroughHierarchyChangeListener mPassThroughListener;

    /** {@inheritDoc} */
    public CustomTabLayout(Context context) {
        super(context);
        setOrientation(VERTICAL);
        init();
    }

    /** {@inheritDoc} */
    public CustomTabLayout(Context context, AttributeSet attrs) {
        super(context, attrs);

        // retrieve selected radio button as requested by the user in the
        // XML layout file
        // TODO: fix ignored attributes
        //        TypedArray attributes = context.obtainStyledAttributes(
        //                attrs, com.android.internal.R.styleable.RadioGroup,
        // com.android.internal.R.attr.radioButtonStyle, 0);

        //        int value =
        // attributes.getResourceId(com.android.internal.R.styleable.RadioGroup_checkedButton,
        // View.NO_ID);
        //        if (value != View.NO_ID) {
        //            mCheckedId = value;
        //        }

        //        final int index =
        // attributes.getInt(com.android.internal.R.styleable.RadioGroup_orientation, VERTICAL);
        //        setOrientation(index);

        //        attributes.recycle();
        init();
    }

    private void init() {
        mChildOnCheckedChangeListener = new CheckedStateTracker();
        mPassThroughListener = new PassThroughHierarchyChangeListener();
        super.setOnHierarchyChangeListener(mPassThroughListener);
    }

    /** {@inheritDoc} */
    @Override
    public void setOnHierarchyChangeListener(OnHierarchyChangeListener listener) {
        // the user listener is delegated to our pass-through listener
        mPassThroughListener.mOnHierarchyChangeListener = listener;
    }

    /** {@inheritDoc} */
    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        // checks the appropriate radio button as requested in the XML file
        if (mCheckedId != -1) {
            mProtectFromCheckedChange = true;
            setCheckedStateForView(mCheckedId, true);
            mProtectFromCheckedChange = false;
            setCheckedId(mCheckedId);
        }
    }

    @Override
    public void addView(View child, int index, ViewGroup.LayoutParams params) {
        if (child instanceof RadioButton) {
            final RadioButton button = (RadioButton) child;
            if (button.isChecked()) {
                mProtectFromCheckedChange = true;
                if (mCheckedId != -1) {
                    setCheckedStateForView(mCheckedId, false);
                }
                mProtectFromCheckedChange = false;
                setCheckedId(button.getId());
            }
        }

        super.addView(child, index, params);
    }

    /**
     * Sets the selection to the radio button whose identifier is passed in parameter. Using -1 as
     * the selection identifier clears the selection; such an operation is equivalent to invoking
     * {@link #clearCheck()}.
     *
     * @param id the unique id of the radio button to select in this group
     * @see #getCheckedRadioButtonId()
     * @see #clearCheck()
     */
    public void check(@IdRes int id) {
        // don't even bother
        if (id != -1 && (id == mCheckedId)) {
            return;
        }

        if (mCheckedId != -1) {
            setCheckedStateForView(mCheckedId, false);
        }

        if (id != -1) {
            setCheckedStateForView(id, true);
        }

        setCheckedId(id);
    }

    private void setCheckedId(@IdRes int id) {
        mCheckedId = id;
        if (mOnCheckedChangeListener != null) {
            mOnCheckedChangeListener.onCheckedChanged(this, mCheckedId);
        }
    }

    private void setCheckedStateForView(int viewId, boolean checked) {
        View checkedView = findViewById(viewId);
        if (checkedView != null && checkedView instanceof RadioButton) {
            ((RadioButton) checkedView).setChecked(checked);
        }
    }

    /**
     * Returns the identifier of the selected radio button in this group. Upon empty selection, the
     * returned value is -1.
     *
     * @return the unique id of the selected radio button in this group
     * @attr ref android.R.styleable#RadioGroup_checkedButton
     * @see #check(int)
     * @see #clearCheck()
     */
    @IdRes
    public int getCheckedRadioButtonId() {
        return mCheckedId;
    }

    /**
     * Clears the selection. When the selection is cleared, no radio button in this group is
     * selected and {@link #getCheckedRadioButtonId()} returns null.
     *
     * @see #check(int)
     * @see #getCheckedRadioButtonId()
     */
    public void clearCheck() {
        check(-1);
    }

    /**
     * Register a callback to be invoked when the checked radio button changes in this group.
     *
     * @param listener the callback to call on checked state change
     */
    public void setOnCheckedChangeListener(OnCheckedChangeListener listener) {
        mOnCheckedChangeListener = listener;
    }

    /** {@inheritDoc} */
    @Override
    public LayoutParams generateLayoutParams(AttributeSet attrs) {
        return new CustomTabLayout.LayoutParams(getContext(), attrs);
    }

    /** {@inheritDoc} */
    @Override
    protected boolean checkLayoutParams(ViewGroup.LayoutParams p) {
        return p instanceof ViewGroup.LayoutParams;
    }

    @Override
    protected LinearLayout.LayoutParams generateDefaultLayoutParams() {
        return new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
    }

    @Override
    public CharSequence getAccessibilityClassName() {
        return CustomTabLayout.class.getName();
    }

    /**
     * This set of layout parameters defaults the width and the height of the children to {@link
     * #WRAP_CONTENT} when they are not specified in the XML file. Otherwise, this class ussed the
     * value read from the XML file.
     *
     * <p>
     *
     * <p>See {@link com.android.internal.R.styleable#LinearLayout_Layout LinearLayout Attributes}
     * for a list of all child view attributes that this class supports.
     */
    public static class LayoutParams extends LinearLayout.LayoutParams {
        /** {@inheritDoc} */
        public LayoutParams(Context c, AttributeSet attrs) {
            super(c, attrs);
        }

        /** {@inheritDoc} */
        public LayoutParams(int w, int h) {
            super(w, h);
        }

        /** {@inheritDoc} */
        public LayoutParams(int w, int h, float initWeight) {
            super(w, h, initWeight);
        }

        /** {@inheritDoc} */
        public LayoutParams(ViewGroup.LayoutParams p) {
            super(p);
        }

        /** {@inheritDoc} */
        public LayoutParams(MarginLayoutParams source) {
            super(source);
        }

        /**
         * Fixes the child's width to {@link android.view.ViewGroup.LayoutParams#WRAP_CONTENT} and
         * the child's height to {@link android.view.ViewGroup.LayoutParams#WRAP_CONTENT} when not
         * specified in the XML file.
         *
         * @param a the styled attributes set
         * @param widthAttr the width attribute to fetch
         * @param heightAttr the height attribute to fetch
         */
        @Override
        protected void setBaseAttributes(TypedArray a, int widthAttr, int heightAttr) {

            if (a.hasValue(widthAttr)) {
                width = a.getLayoutDimension(widthAttr, "layout_width");
            } else {
                width = WRAP_CONTENT;
            }

            if (a.hasValue(heightAttr)) {
                height = a.getLayoutDimension(heightAttr, "layout_height");
            } else {
                height = WRAP_CONTENT;
            }
        }
    }

    /**
     * Interface definition for a callback to be invoked when the checked radio button changed in
     * this group.
     */
    public interface OnCheckedChangeListener {
        /**
         * Called when the checked radio button has changed. When the selection is cleared,
         * checkedId is -1.
         *
         * @param group the group in which the checked radio button has changed
         * @param checkedId the unique identifier of the newly checked radio button
         */
        public void onCheckedChanged(CustomTabLayout group, @IdRes int checkedId);
    }

    private class CheckedStateTracker implements CompoundButton.OnCheckedChangeListener {
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
            // prevents from infinite recursion
            if (mProtectFromCheckedChange) {
                return;
            }

            mProtectFromCheckedChange = true;
            if (mCheckedId != -1) {
                setCheckedStateForView(mCheckedId, false);
            }
            mProtectFromCheckedChange = false;

            int id = buttonView.getId();
            setCheckedId(id);
        }
    }

    /**
     * A pass-through listener acts upon the events and dispatches them to another listener. This
     * allows the table layout to set its own internal hierarchy change listener without preventing
     * the user to setup his.
     */
    private class PassThroughHierarchyChangeListener
            implements ViewGroup.OnHierarchyChangeListener {
        private ViewGroup.OnHierarchyChangeListener mOnHierarchyChangeListener;

        public void traverseTree(View view) {
            if (view instanceof RadioButton) {
                int id = view.getId();
                // generates an id if it's missing
                if (id == View.NO_ID) {
                    id = View.generateViewId();
                    view.setId(id);
                }
                ((RadioButton) view).setOnCheckedChangeListener(mChildOnCheckedChangeListener);
            }
            if (!(view instanceof ViewGroup)) {
                return;
            }
            ViewGroup viewGroup = (ViewGroup) view;
            if (viewGroup.getChildCount() == 0) {
                return;
            }
            for (int i = 0; i < viewGroup.getChildCount(); i++) {
                traverseTree(viewGroup.getChildAt(i));
            }
        }

        /** {@inheritDoc} */
        public void onChildViewAdded(View parent, View child) {
            traverseTree(child);
            if (parent == CustomTabLayout.this && child instanceof RadioButton) {
                int id = child.getId();
                // generates an id if it's missing
                if (id == View.NO_ID) {
                    id = View.generateViewId();
                    child.setId(id);
                }
                ((RadioButton) child).setOnCheckedChangeListener(mChildOnCheckedChangeListener);
            }

            if (mOnHierarchyChangeListener != null) {
                mOnHierarchyChangeListener.onChildViewAdded(parent, child);
            }
        }

        /** {@inheritDoc} */
        public void onChildViewRemoved(View parent, View child) {
            if (parent == CustomTabLayout.this && child instanceof RadioButton) {
                ((RadioButton) child).setOnCheckedChangeListener(null);
            }

            if (mOnHierarchyChangeListener != null) {
                mOnHierarchyChangeListener.onChildViewRemoved(parent, child);
            }
        }
    }
}
